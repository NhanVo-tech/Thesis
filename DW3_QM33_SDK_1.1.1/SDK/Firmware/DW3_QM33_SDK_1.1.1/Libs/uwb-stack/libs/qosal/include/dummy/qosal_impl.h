/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#ifndef QOSAL_IMPL_H
#define QOSAL_IMPL_H

/**
 * Value is hardcoded due to limitation when building uwb-stack as library.
 * Please ensure to align configuration of your OS with this value.
 */
#define QOSAL_IMPL_THREAD_MAX_NAME_LEN 12

/*
 * Hack for compilation llhw as library.
 * The current build system architecture doesn't give robust
 * mechanisms for customize qosal for FreeRTOS while compiling as library
 * (qosal is compiled as dummy)
 */
#define QOSAL_IMPL_THREAD_STACK_DEFINE(name, stack_size) static uint8_t name[stack_size]

#define QOSAL_IMPL_LOG_INFO(...)
#define QOSAL_IMPL_LOG_ERR(...)
#define QOSAL_IMPL_LOG_WARN(...)
#define QOSAL_IMPL_LOG_DEBUG(...)

#define QOSAL_IMPL_IRQ_CONNECT(irqn, prio, handler)

/* The following functions are valid on all ARM Cortex-M. */
#define QOSAL_IMPL_IRQ_ENABLE(irqn) NVIC_EnableIRQ(irqn)
#define QOSAL_IMPL_IRQ_DISABLE(irqn) NVIC_DisableIRQ(irqn)
#define QOSAL_IMPL_IRQ_CLEAR_PENDING(irqn) NVIC_ClearPendingIRQ(irqn)
#define QOSAL_IMPL_IRQ_GET(irqn) NVIC_GetEnableIRQ(irqn)

#endif /* QOSAL_IMPL_H */
