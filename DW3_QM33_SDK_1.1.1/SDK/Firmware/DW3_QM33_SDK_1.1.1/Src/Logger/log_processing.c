/**
 * @file      log_processing.c
 *
 * @brief     Log processing task
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "log_processing.h"

#include <nrf_log_ctrl.h>
#include <nrf_log_default_backends.h>
#include <qassert.h>
#include "qtime.h"
#include "qrtc.h"
#include "qsignal.h"
#include "qthread.h"
#include "qmalloc.h"
#include "qlog.h"
#include "int_priority.h"
#include "deca_error.h"

#define LOG_FLUSH_MS (5)

#if defined(CONFIG_LOG) && defined(NRF_LOG_ENABLED)

static struct qthread *log_thread;        /* qthread handler. */
static uint8_t *log_task_stack;           /* Pointer to Task stack. */
static struct qsignal *log_signal = NULL; /* Signal to notify log available */

static uint32_t get_timestamp(void)
{
    uint32_t timestamp = qrtc_get_us();

    return timestamp;
}

static void init_log(void)
{
    ret_code_t err_code = NRF_LOG_INIT(get_timestamp);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

static void log_processing_task(void *pvParameters)
{
    while (1)
    {
        bool are_log_available = false;
        if (qsignal_wait(log_signal, (void *)&are_log_available, QOSAL_WAIT_FOREVER) == QERR_SUCCESS)
        {
            NRF_LOG_FLUSH();
        }
    }
}

void log_pending_hook(void)
{
    if (log_signal)
    {
        qsignal_raise(log_signal, true);
    }
}

void create_log_processing_task(void)
{
    init_log();

    /* Initialize Log signal */
    log_signal = qsignal_init();
    if (!log_signal)
    {
        return;
    }

    size_t task_size = 768;
    log_task_stack = qmalloc(task_size);

    /* Create the task, storing the handle. */
    log_thread = qthread_create(log_processing_task, NULL, "Logger", log_task_stack, task_size, PRIO_LogProcessingTask);
    if (!log_thread)
    {
        qfree(log_task_stack);
        qsignal_deinit(log_signal);
        return;
    }
}

#endif
