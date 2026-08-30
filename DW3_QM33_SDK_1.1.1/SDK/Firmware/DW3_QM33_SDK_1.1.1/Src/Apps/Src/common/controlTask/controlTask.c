/**
 * @file      controlTask.c
 *
 * @brief     Control task for USB/UART
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "usb_uart_rx.h"
#include "int_priority.h"
#include "circular_buffer.h"
#include "cmd_fn.h"
#include "HAL_error.h"
#include "task_signal.h"
#include "qtime.h"
#include "qmalloc.h"
#include "qmsg_queue.h"
#include "qlog.h"

#ifdef UCI_BUILD
#define CONTROL_TASK_STACK_SIZE_BYTES 512
#else
#define CONTROL_TASK_STACK_SIZE_BYTES 2048
#endif

/* Signal value when data is received. */
#define CTRL_DATA_RECEIVED 0x02

/* Signal value when STOP is required. */
#define CTRL_STOP_APP 0x04

/* Control task message definition. */
struct ctrl_mail_data
{
    uint32_t message_type;
    void *data;
};

#define CTRL_QUEUE_ITEM_SIZE sizeof(struct ctrl_mail_data)
#define CTRL_QUEUE_MAX_ITEM  5
#define CTRL_QUEUE_SIZE      (CTRL_QUEUE_ITEM_SIZE * CTRL_QUEUE_MAX_ITEM)

static struct qmsg_queue *ctrl_msg_queue = NULL;
static char uci_queue_buffer[CTRL_QUEUE_SIZE];

/* Control task handler. */
task_signal_t ctrlTask;

#ifdef CLI_BUILD
/* Command buffer used between on_rx and command_parser. */
#define MAX_CMD_LENGTH 0x200
char cmdBuf[MAX_CMD_LENGTH];
#endif

/**
 * @brief This is a Command Control and Data task to handle received data.
 * This task is activated on the startup.
 * There are 2 sources of control data: Uart and Usb.
 * */
static void CtrlTask(void *arg)
{
    usb_data_e res;

    /* Initialize queue */
    if (!ctrl_msg_queue)
        ctrl_msg_queue = qmsg_queue_init(uci_queue_buffer, CTRL_QUEUE_ITEM_SIZE, CTRL_QUEUE_MAX_ITEM);
    assert(ctrl_msg_queue);

    while (1)
    {
        struct ctrl_mail_data msg;
        /* Wait data from USB/UART. */
        if (qmsg_queue_get(ctrl_msg_queue, &msg, QOSAL_WAIT_FOREVER) == QERR_SUCCESS)
        {
            void *command_parser_data;
#ifdef CLI_BUILD
            command_parser_data = cmdBuf;
#else
            command_parser_data = msg.data;
#endif
            switch (msg.message_type)
            {
#ifdef CLI_BUILD
                case CTRL_STOP_APP:
                    /* Time for flushing, TODO: create a function for testing flushing is over. */
                    qtime_msleep(500);
                    command_stop_received();
                    break;
#endif
                case CTRL_DATA_RECEIVED:
                    res = usb_uart_rx((struct cc_buff *)msg.data);
                    AppGet()->command_parser(res, command_parser_data);
                    break;
                default:
                    QLOGW("CONTROL: Unknown message type %u", msg.message_type);
            }
        }
    }
}

/**
 * @brief Creation of Control task and signal.
 * Note. The Control task awaits an input on a USB and/or UART interfaces.
 */
void ControlTaskInit(void)
{
    /* Create Control Task. */
    size_t task_size = CONTROL_TASK_STACK_SIZE_BYTES;
    ctrlTask.task_stack = qmalloc(task_size);

    ctrlTask.thread = qthread_create(CtrlTask, NULL, "Control", ctrlTask.task_stack, task_size, PRIO_CtrlTask);
    if (!ctrlTask.thread)
    {
        error_handler(1, _ERR_Create_Task_Bad);
    }
}

/** @brief Notify that a new data is received. */
void NotifyControlTaskData(struct cc_buff *buf)
{
    /* Send message only if Control Task is started. */
    if (ctrlTask.thread != NULL)
    {
        struct ctrl_mail_data msg = {.message_type = CTRL_DATA_RECEIVED, .data = buf};

        qmsg_queue_put(ctrl_msg_queue, &msg);
    }
}

/** @brief Notify that STOP App is required. */
void NotifyControlTaskStopApp(void)
{
    /* Send message only if Control Task is started. */
    if (ctrlTask.thread != NULL)
    {
        struct ctrl_mail_data msg = {};
        msg.message_type = CTRL_STOP_APP;
        qmsg_queue_put(ctrl_msg_queue, &msg);
    }
}
