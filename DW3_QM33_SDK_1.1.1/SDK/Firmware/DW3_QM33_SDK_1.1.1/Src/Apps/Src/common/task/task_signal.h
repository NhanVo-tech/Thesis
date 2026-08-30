/**
 * @file      task_signal.h
 *
 * @brief     Interface for apps task signals
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "qthread.h"
#include "qsignal.h"

/* Application tasks handles & corresponded signals structure. */
struct task_signal_s
{
    struct qthread *thread; /* qthread handler. */
    struct qsignal *signal; /* qsignal handler. */

    uint8_t *task_stack; /* Pointer on Task stack. */
    volatile int Exit;   /* Task's exit flag 0=stay in loop, 1=exit loop, 2=loop exited. */
};

typedef struct task_signal_s task_signal_t;

#define STOP_TASK 0x01 /* Common to all tasks. */

bool terminate_task(struct task_signal_s *task);

#ifdef __cplusplus
}
#endif
