/**
 * @file      task_signal.c
 *
 * @brief     Task signals
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "task_signal.h"

#include "qmalloc.h"
#include "qtime.h"
#include "qthread.h"
#include "qsignal.h"
#include "qirq.h"

bool terminate_task(struct task_signal_s *task)
{
    bool ret = false;
    if (task->thread)
    {
        task->Exit = 1;

        if (!task->signal)
        {
            qsignal_raise(task->signal, STOP_TASK);
            while (task->Exit != 2)
            {
                qtime_msleep(1);
            }
        }

        unsigned int lock = qirq_lock();
        if (qthread_delete(task->thread) == QERR_SUCCESS)
        {
            task->thread = NULL;
            ret = true;
        }

        if (task->signal)
        {
            qsignal_deinit(task->signal);
            task->signal = NULL;
        }

        if (task->task_stack)
        {
            qfree(task->task_stack);
            task->task_stack = NULL;
        }
        qirq_unlock(lock);
    }
    return ret;
}
