/**
 * @file      int_priority.h
 *
 * @brief     Task and interrupt definitions
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include "custom_board.h"
#include "qthread.h"
#include "FreeRTOSConfig.h"

enum Priorities
{
    /* Interrupt priorities. */
    PRIO_RTC_WKUP_IRQn = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, /* Equivalent to the highest in the FreeRTOS. */

    /* Application-specific priorities. */
    PRIO_FlushTask = QTHREAD_PRIORITY_ABOVE_NORMAL,
    PRIO_CtrlTask = QTHREAD_PRIORITY_NORMAL,
    PRIO_StartDefaultTask = QTHREAD_PRIORITY_LOW,
    PRIO_RxTask = QTHREAD_PRIORITY_HIGH,
    PRIO_UciTask = QTHREAD_PRIORITY_NORMAL,
    PRIO_LogProcessingTask = QTHREAD_PRIORITY_IDLE
};
