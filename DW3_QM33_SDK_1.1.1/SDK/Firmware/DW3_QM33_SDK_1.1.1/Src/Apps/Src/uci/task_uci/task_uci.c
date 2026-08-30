/**
 * @file      task_uci.c
 *
 * @brief     uci example task
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "task_uci.h"

#include <stdbool.h>

#include "controlTask.h"
#include "uci_backend/default_coordinator.h"
#include "uci_backend/uci_backend_mac.h"
#include "uci_backend/uci_backend_mac_calibrations.h"
#include "uci_backend/uci_backend_pctt.h"
#include "uci_backend/uci_backend_cfg_mgr.h"
#include "uci_parser.h"
#include "uci_transport.h"
#include "uwbmac_helper.h"
#include "rtls_version.h"
#include "qmsg_queue.h"
#include "task_signal.h"
#include "int_priority.h"
#include "HAL_error.h"
#include "qlog.h"
#include "qmutex.h"
#include "uwb_utils.h"

#define UCI_TASK_STACK_SIZE_BYTES (6 * 1024)
#define UCI_RX_BUF_SIZE           1024

#define UCI_QUEUE_ITEM_SIZE       sizeof(struct uci_mail_data)
#define UCI_QUEUE_MAX_ITEM        5
#define UCI_QUEUE_SIZE            (UCI_QUEUE_ITEM_SIZE * UCI_QUEUE_MAX_ITEM)

static task_signal_t uciTask;
static struct qmsg_queue *uci_msg_queue = NULL;

extern const app_definition_t helpers_uci_node[];

static void uci_reset_cb(uint8_t reason, void *user_data);
static void uci_task_process_incoming(struct cc_buff *buf);

struct uci_mail_data
{
    uint32_t message_type;
    struct cc_buff *buf;
};

static char uci_queue_buffer[UCI_QUEUE_SIZE];

static const struct uci_transport_ops uci_usb_ops = {
    .attach = uci_tp_attach,
    .detach = uci_tp_detach,
    .packet_send_ready = uci_tp_usb_packet_send_ready,
};

static struct uci_tp tr = {
    .tr = {.ops = NULL},
    .uci = NULL,
    .uci_if = UCI_NONE,
};

static struct uci_blk *simple_alloc(struct uci_allocator *uci, size_t size_hint, uint8_t __attribute__((unused)) flags_hint)
{
    struct uci_blk *ret;

    ret = (struct uci_blk *)qmalloc(sizeof(*ret) + size_hint);
    if (ret)
    {
        ret->data = (uint8_t *)&ret[1];
        ret->size = UCI_MAX_PACKET_SIZE;
        ret->len = 0;
    }

    return ret;
}

static void simple_free(struct uci_allocator *uci, struct uci_blk *b)
{
    qfree(b);
}

static const struct uci_allocator_ops simple_allocator_ops = {
    .alloc = simple_alloc,
    .free = simple_free,
};

static const struct uci_allocator simple_allocator = {
    .ops = &simple_allocator_ops};

static struct
{
    struct uwbmac_context *uwbmac_ctx;
    struct uci uci_server;
    struct uci_backend_core_context core_ctx;
    struct default_coordinator coord;
    struct uci_backend_fira_context fira_ctx;
#ifdef UCI_MAC_BACKEND
    struct uci_backend_mac_context mac_ctx;
#endif
#ifdef UCI_FTM_BACKEND
    struct uci_backend_pctt_context pctt_ctx;
#endif
    struct uci_backend_manager sess_man;
#ifdef UCI_MAC_CALIB_BACKEND
    struct uci_backend_mac_calibrations_context mac_cal_ctx;
#endif
} ctx;

struct uci_backend_core_ops uci_backend_fira_ops = {
    .device_reset = uci_reset_cb,
    .get_caps_info = NULL,
    .ops_list_next = NULL,
    .user_data = NULL

};

#define SOC_INFO_LEN (32)
struct __attribute__((__aligned__(4), __packed__)) vendor_data_struct
{
    uint8_t qmf_version_major;
    uint8_t qmf_version_minor;
    uint8_t qmf_version_patch;
    uint8_t qmf_version_pre_release;
    uint64_t qmf_version_extra;
    uint8_t oem_version_major;
    uint8_t oem_version_minor;
    uint8_t oem_version_revision;
    uint8_t soc_info[SOC_INFO_LEN];
    uint32_t dev_id;
};

static struct vendor_data_struct vendor_data = {0};

void uci_interface_select_uart0(void)
{
    tr.uci_if = UCI_UART0;
    tr.tr.ops = &uci_usb_ops;
}

void uci_reset_cb(uint8_t reason, void *user_data)
{
    bool status = uci_sw_reset();
    uci_backend_core_reset_done(&ctx.core_ctx, status);
}

int qm_erase_certificates(void)
{
    return 0;
}

static void qm33_soc_info(uint8_t soc_info[], uint64_t lot_id, uint32_t part_id)
{
    soc_info[0] = (lot_id >> 56) & 0xFF;
    soc_info[1] = (lot_id >> 48) & 0xFF;
    soc_info[2] = (lot_id >> 40) & 0xFF;
    soc_info[3] = (lot_id >> 32) & 0xFF;
    soc_info[4] = (lot_id >> 24) & 0xFF;
    soc_info[5] = (lot_id >> 16) & 0xFF;
    soc_info[6] = (lot_id >> 8) & 0xFF;
    soc_info[7] = lot_id & 0xFF;

    soc_info[8] = (part_id >> 24) & 0xFF;
    soc_info[9] = (part_id >> 16) & 0xFF;
    soc_info[10] = (part_id >> 8) & 0xFF;
    soc_info[11] = part_id & 0xFF;
}

void uci_task_on_entry(void)
{
    /* Initialize queue */
    if (!uci_msg_queue)
        uci_msg_queue = qmsg_queue_init(uci_queue_buffer, UCI_QUEUE_ITEM_SIZE, UCI_QUEUE_MAX_ITEM);
    assert(uci_msg_queue);

    uciTask.Exit = 0;

    /* Initialize UWB driver for FiRa by default. */
    uwbmac_helper_init_fira();
    uci_open_backends();
}

/**
 * @brief this starts the UCI functionality.
 * @note Previous tasks which can call shared resources must be killed.
 * This task needs the RAM size of at least uci_t.
 */
void uci_task(void *argument)
{
    uci_task_on_entry();

    /* Reset default interface. */
    tr.uci_if = UCI_NONE;

    while (uciTask.Exit == 0)
    {
        struct uci_mail_data msg;

        if (qmsg_queue_get(uci_msg_queue, &msg, QOSAL_WAIT_FOREVER) == QERR_SUCCESS)
        {
            switch (msg.message_type)
            {
                case UCI_DATA_IN:
                    uci_task_process_incoming(msg.buf);
                    break;

                case UCI_DATA_OUT:
                    break;

                default:
                    break;
            }
        }
    }

    uciTask.Exit = 2;
    while (uciTask.Exit == 2)
    {
        qtime_msleep(1);
    };
}

void uci_task_process_incoming(struct cc_buff *buf)
{
    uci_tp_read(&tr, buf);
}

/**
 * @brief Kill all tasks and timers related to uci if any
 */
void uci_terminate(void)
{
    terminate_task(&uciTask);
    uci_uninit(&ctx.uci_server);
    uci_process_terminate();
    uci_close_backends();
    uwbmac_helper_deinit();
}

/**
 * @brief stop function implements the stop functionality if any suitable for current process.
 */
void uci_process_terminate(void)
{
    tr.uci_if = UCI_NONE;
}

/** @brief this is a service function which starts the uci test functionality. */
void uci_helper(void const *arg)
{
    /* Reset default interface. */
    tr.uci_if = UCI_NONE;

    /* Create UCI task */
    size_t task_size = UCI_TASK_STACK_SIZE_BYTES;
    uciTask.task_stack = qmalloc(task_size);

    uciTask.thread = qthread_create((qthread_func)uci_task, (void *)arg, "UCI", uciTask.task_stack, task_size, PRIO_UciTask);
    if (!uciTask.thread)
    {
        error_handler(1, _ERR_Create_Task_Bad);
    }
}

void uci_notify_data_in(usb_data_e res, void *data)
{
    struct cc_buff *buf = (struct cc_buff *)data;
    struct uci_mail_data msg = {.message_type = UCI_DATA_IN, .buf = buf};

    uci_interface_select();
    qmsg_queue_put(uci_msg_queue, &msg);
}

void uci_interface_select()
{
    if (tr.uci_if == UCI_NONE)
    {
        uci_interface_select_uart0();
        uci_transport_attach(&ctx.uci_server, &tr.tr);
    }
}

bool uci_sw_reset()
{
    bool status = true;

    uci_close_backends();
    if (uci_open_backends())
    {
        /* Error. */
        status = false;
    }
    return status;
}

int uci_open_backends()
{
    int err;

    /* Initialize MAC. */
    err = uwbmac_init(&ctx.uwbmac_ctx);
    if (err)
    {
        QLOGE("Failed to init uwbmac, err: %d", err);
        return err;
    }
    /* Initialize UCI server */
    err = uci_init(&ctx.uci_server, (struct uci_allocator *)&simple_allocator, false);
    if (err)
    {
        QLOGE("Failed to init uci server, err %d", err);
        return err;
    }

    /* Send device ready notification on uart0. */
    uci_interface_select_uart0();
    uci_transport_attach(&ctx.uci_server, &tr.tr);
    uci_reset_device_state(&ctx.uci_server);

    /* Initialize coordinator and session manager. */
    default_coordinator_init(&ctx.coord, ctx.uwbmac_ctx);

    uci_backend_core_init(&ctx.core_ctx, &ctx.uci_server, ctx.uwbmac_ctx);

    /* Initialize fira/CCC backend. */
    uci_backend_manager_init(&ctx.sess_man, &ctx.uci_server, ctx.uwbmac_ctx);

#ifdef UCI_FIRA_BACKEND
    /* Initialize FiRa backend. */
    err = uci_backend_fira_init(&ctx.fira_ctx, &ctx.core_ctx, &ctx.uci_server, ctx.uwbmac_ctx, &ctx.coord.base, &ctx.sess_man);
    if (err)
    {
        QLOGE("Failed to initialize fira backend, err %d", err);
        return err;
    }

    struct uwbmac_device_info device_info;
    err = uwbmac_get_device_info(ctx.uwbmac_ctx, &device_info);
    if (err)
    {
        QLOGE("Failed to get the device info, err %d", err);
        return err;
    }

    const aoa_supported_t is_aoa = uwb_device_is_aoa(device_info.dev_id);
    if (is_aoa == AOA_SUPPORTED)
    {
        ctx.fira_ctx.antennas->aoa_capability = 1;
    }
    else if (is_aoa == AOA_NOT_SUPPORTED)
    {
        ctx.fira_ctx.antennas->aoa_capability = 0;
    }
    else
    {
        QLOGE("Device ID is not recognized.");
        return QERR_ENOTSUP;
    }
#endif

#ifdef UCI_FTM_BACKEND
    /* Initialize FTM backend. */
    err = uci_backend_pctt_init(&ctx.pctt_ctx, &ctx.uci_server, ctx.uwbmac_ctx, &ctx.coord.base, &ctx.sess_man);
    if (err)
    {
        QLOGE("Failed to initialize pctt backend, err %d", err);
        return err;
    }
#endif

#ifdef UCI_CONF_MANAGER
    uci_backend_cfg_mgr_register(&ctx.uci_server, ctx.uwbmac_ctx);
#endif

#ifdef UCI_MAC_BACKEND
    err = uci_backend_mac_init(&ctx.mac_ctx, &ctx.uci_server, ctx.uwbmac_ctx, NULL, NULL);
    if (err)
    {
        QLOGE("Failed to initialize mac backend, err %d", err);
        return err;
    }
#endif

#ifdef UCI_MAC_CALIB_BACKEND
    err = uci_backend_mac_calibrations_init(&ctx.mac_cal_ctx, &ctx.uci_server, ctx.uwbmac_ctx);
    if (err)
    {
        QLOGE("Failed to initialize mac calibrations backend, err %d", err);
        return err;
    }
#endif
    uci_backend_core_register_ops(&ctx.core_ctx, &uci_backend_fira_ops, NULL);

    /* Set vendor specific data */
    vendor_data.qmf_version_major = VER_MAJOR;
    vendor_data.qmf_version_minor = VER_MINOR;
    vendor_data.qmf_version_patch = VER_PATCH;
    qm33_soc_info(vendor_data.soc_info, device_info.lot_id, device_info.part_id);
    vendor_data.dev_id = device_info.dev_id;
    uci_backend_core_set_vendor_data(&ctx.core_ctx, (const uint8_t *)&vendor_data, sizeof(vendor_data));
    return 0;
}

void uci_close_backends()
{
#ifdef UCI_FIRA_BACKEND
    uci_backend_fira_release(&ctx.fira_ctx);
#endif

#ifdef UCI_FTM_BACKEND
    uci_backend_pctt_release(&ctx.pctt_ctx);
#endif

#ifdef UCI_CONF_MANAGER
    uci_backend_cfg_mgr_unregister();
#endif

#ifdef UCI_MAC_BACKEND
    uci_backend_mac_release(&ctx.mac_ctx);
#endif

    uci_backend_manager_release(&ctx.sess_man);

    uci_backend_core_release(&ctx.core_ctx);

    /* We are stopping everything, we will send the device state NTF manually. */
    uwbmac_register_device_state_callback(ctx.uwbmac_ctx, NULL, NULL);
    default_coordinator_init(&ctx.coord, ctx.uwbmac_ctx);
    uci_uninit(&ctx.uci_server);

    uwbmac_exit(ctx.uwbmac_ctx);
}

const app_definition_t helpers_uci_node[] __attribute__((section(".known_apps"))) = {
    {"UCI", mAPP, uci_helper, uci_terminate, uci_on_rx, uci_notify_data_in, NULL}};
