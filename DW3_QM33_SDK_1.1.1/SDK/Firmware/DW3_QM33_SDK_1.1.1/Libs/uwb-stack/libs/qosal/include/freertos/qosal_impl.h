/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include "FreeRTOSConfig.h"

#define QOSAL_IMPL_THREAD_MAX_NAME_LEN configMAX_TASK_NAME_LEN
#define QOSAL_IMPL_THREAD_STACK_DEFINE(name, stack_size) static uint8_t name[stack_size]

#ifdef CONFIG_LOG
#include <qlog_impl.h>
#else
#define QOSAL_IMPL_LOG_INFO(...)
#define QOSAL_IMPL_LOG_ERR(...)
#define QOSAL_IMPL_LOG_WARN(...)
#define QOSAL_IMPL_LOG_DEBUG(...)
#endif

#define QOSAL_IMPL_IRQ_CONNECT(irqn, prio, handler)

/* The following functions are valid on all ARM Cortex-M. */
#define QOSAL_IMPL_IRQ_ENABLE(irqn) NVIC_EnableIRQ(irqn)
#define QOSAL_IMPL_IRQ_DISABLE(irqn) NVIC_DisableIRQ(irqn)
#define QOSAL_IMPL_IRQ_CLEAR_PENDING(irqn) NVIC_ClearPendingIRQ(irqn)
#define QOSAL_IMPL_IRQ_GET(irqn) NVIC_GetEnableIRQ(irqn)
