/**
 * @file      hello_world.c
 *
 * @brief     Implementation of helloWorld application
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */
#include "hello_world.h"

#include "qthread.h"
#include "qmalloc.h"
#include "qlog.h"

#include "deca_error.h"
#include "qplatform.h"
#include "llhw.h"
#include "uwbmac/uwbmac.h"
#include "persistent_time.h"

#include <stdio.h>

#define HELLO_WORLD_TASK_STACK_SIZE_BYTES 1024

#define NUM_BUFFERS                       4
#define BUFFER_SIZE                       128

extern struct l1_config_platform_ops l1_config_platform_ops;
static enum qerr uwb_stack_init(struct uwbmac_context **uwbmac_ctx)
{
    enum qerr r;

    r = qplatform_init();
    if (r != QERR_SUCCESS)
        return r;

    r = l1_config_init(&l1_config_platform_ops);
    if (r != QERR_SUCCESS)
        goto deinit_qplatform;

    r = llhw_init();
    if (r != QERR_SUCCESS)
        goto deinit_l1_config;

    r = uwbmac_init(uwbmac_ctx);

    /* Success. */
    if (r == QERR_SUCCESS)
        goto exit;

    llhw_deinit();
deinit_l1_config:
    l1_config_deinit();
deinit_qplatform:
    qplatform_deinit();
exit:
    return r;
}

static void uwb_stack_deinit(struct uwbmac_context *uwbmac_ctx)
{
    uwbmac_exit(uwbmac_ctx);
    llhw_deinit();
    l1_config_deinit();
    qplatform_deinit();
}

static void hello_world_task(void *arg)
{
    static char buf[NUM_BUFFERS][BUFFER_SIZE];
    struct uwbmac_context *uwbmac_ctx = NULL;
    struct uwbmac_device_info device_info;

    /* Initialize persistant time base. */
    persistent_time_init(0);

    enum qerr err = uwb_stack_init(&uwbmac_ctx);
    if (err != QERR_SUCCESS)
    {
        QLOGE("Failed to init UWB stack.");
        return;
    }

    uint8_t retry_count = 3;
    do
    {
        err = uwbmac_get_device_info(uwbmac_ctx, &device_info);
        if (err != QERR_SUCCESS)
            QLOGW("Retrying... Attempts left: %d", retry_count - 1);
    } while ((err != QERR_SUCCESS) && --retry_count);

    uwb_stack_deinit(uwbmac_ctx);

    if (err != QERR_SUCCESS)
    {
        QLOGE("Failed to get device info.");
        return;
    }

    snprintf(buf[0], BUFFER_SIZE, "Qorvo Device ID = 0x%08lx", device_info.dev_id);
    snprintf(buf[1], BUFFER_SIZE, "Qorvo Lot ID = 0x%08lx%08lx", (uint32_t)(device_info.lot_id >> 32), (uint32_t)device_info.lot_id);
    snprintf(buf[2], BUFFER_SIZE, "Qorvo Part ID = 0x%08lx", device_info.part_id);
    snprintf(buf[3], BUFFER_SIZE, "Qorvo SoC ID = %08lx%08lx%08lx", (uint32_t)(device_info.lot_id >> 32), (uint32_t)device_info.lot_id, device_info.part_id);

    QLOGI("Hello World!");

    while (1)
    {
        for (int i = 0; i < 4; i++)
        {
            QLOGI("%s", buf[i]);
        }
        qtime_msleep_yield(1000);
    }
}

error_e hello_world_init(void)
{
    /* Create a hello world task. */
    const size_t task_size = HELLO_WORLD_TASK_STACK_SIZE_BYTES;
    static uint8_t *hello_world_task_stack;
    static struct qthread *hello_world_thread;

    hello_world_task_stack = qmalloc(task_size);
    if (hello_world_task_stack == NULL)
    {
        QLOGE("Failed to allocate memory for HelloWorld task stack.");
        return _ERR_Cannot_Alloc_Memory;
    }

    hello_world_thread = qthread_create(hello_world_task, NULL, "HelloWorld", hello_world_task_stack, HELLO_WORLD_TASK_STACK_SIZE_BYTES, QTHREAD_PRIORITY_NORMAL);
    if (!hello_world_thread)
    {
        QLOGE("Failed to create HelloWorld task.");
        qfree(hello_world_task_stack);
        return _ERR_Create_Task_Bad;
    }

    return _NO_ERR;
}
