/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qosal_impl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * QIRQ_CONNECT() - Configure IRQ priority and handler.
 * @irqn: IRQ number.
 * @prio: IRQ priority.
 * @handler: IRQ handler.
 */
#define QIRQ_CONNECT(irqn, prio, handler) QOSAL_IMPL_IRQ_CONNECT(irqn, prio, handler)

/**
 * QIRQ_ENABLE() - Enable IRQ.
 * @irqn: IRQ number.
 */
#define QIRQ_ENABLE(irqn) QOSAL_IMPL_IRQ_ENABLE(irqn)

/**
 * QIRQ_DISABLE() - Enable IRQ.
 * @irqn: IRQ number.
 */
#define QIRQ_DISABLE(irqn) QOSAL_IMPL_IRQ_DISABLE(irqn)

/**
 * QIRQ_CLEAR_PENDING() - Clear pending IRQ.
 * @irqn: IRQ number.
 */
#define QIRQ_CLEAR_PENDING(irqn) QOSAL_IMPL_IRQ_CLEAR_PENDING(irqn)

/**
 * QIRQ_GET() - Get IRQ.
 * @irqn: IRQ number.
 */
#define QIRQ_GET(irqn) QOSAL_IMPL_IRQ_GET(irqn)

/**
 * qirq_lock() - Disable all interrupts.
 *
 * The use of the lock-out key to be able to nest lock/unlock is not supported depending on the
 * implementation. It is advisable to not use this feature.
 *
 * Return: A lock-out key, its representation is implementation-defined.
 */
unsigned int qirq_lock(void);

/**
 * qirq_unlock() - Enable interrupts previously disabled by qirq_lock().
 *
 * The use of the lock-out key to be able to nest lock/unlock is not supported depending on the
 * implementation.
 * It is advisable to not use this feature.
 *
 * @key: The lock-out key returned by the previous call to qirq_lock().
 */
void qirq_unlock(unsigned int key);

#ifdef __cplusplus
}
#endif
