/*
 * @file      flushTask.c
 *
 * @brief     Flush task
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "app.h"
#include "flushTask.h"

#include "int_priority.h"
#include "usb_uart_tx.h"
#include "HAL_error.h"
#include "HAL_watchdog.h"
#include "qmalloc.h"
#include "qsignal.h"
#include "qthread.h"
#include "qirq.h"

#define FLUSH_TASK_STACK_SIZE_BYTES 512
/* Period of FlushTask */
#define USB_FLUSH_MS 5
/* Handler of flush task */
static struct qthread *flush_task = NULL;
/* Handler of flush signal */
static struct qsignal *flush_signal = NULL;


/**
 * @brief Flush task execution.
 * This thread is flushing report buffer on demand or every USB_FLUSH_MS ms.
 * */
void FlushTask(void *argument)
{
    int last_signal_value = 0;
    while (1)
    {
        Watchdog.refresh();
        qsignal_wait(flush_signal, &last_signal_value, USB_FLUSH_MS);
        /*
         * Flush report buffer in both situation, when Flush notification was received
         * and when timeout was reached.
         */
        flush_report_buf();
    }
}

/** @brief Reset report buffer. */
void FlushTask_reset(void)
{
    if (flush_task)
    {
        unsigned int lock = qirq_lock();
        reset_report_buf();
        qirq_unlock(lock);
    }
}

/** @brief Notify that flush is required. */
void NotifyFlushTask(void)
{
    /* RTOS : flush could be not started yet. */
    if (flush_task)
    {
        qsignal_raise(flush_signal, 0);
    }
}

/**
 * @brief Creation of Flush task.
 * The Flush task is responsible of flushing the output buffer to UART/USB
 * FlushTask is always working.
 */
void FlushTaskInit(void)
{
    /* Create Flush task. */
    size_t task_size = FLUSH_TASK_STACK_SIZE_BYTES;
    uint8_t *task_stack = qmalloc(task_size);

    flush_task = qthread_create(FlushTask, NULL, "Flush", task_stack, task_size, PRIO_FlushTask);
    if (!flush_task)
    {
        error_handler(1, _ERR_Create_Task_Bad);
    }

    /* Create Flush signal. */
    flush_signal = qsignal_init();
    if (!flush_signal)
    {
        error_handler(1, _ERR_Create_Task_Bad);
    }

    /* Initialize tx messages handler. */
    if (init_tx_msg() != _NO_ERR)
    {
        error_handler(1, _ERR_TxBuf_Overflow);
    }
}
