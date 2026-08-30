/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qerr.h>
#include <qtime.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * struct qsemaphore: QOSAL Semaphore (opaque).
 */
struct qsemaphore;

/**
 * qsemaphore_init() - Initialize a semaphore.
 * @init_count: Initial semaphore count.
 * @max_count: Maximum semaphore count.
 *
 * Return: Pointer to the initialized semaphore on NULL on error.
 */
struct qsemaphore *qsemaphore_init(uint32_t init_count, uint32_t max_count);

/**
 * qsemaphore_deinit() - De-initialize a semaphore.
 * @sem: Pointer to the semaphore initialized by qsemaphore_init().
 */
void qsemaphore_deinit(struct qsemaphore *sem);

/**
 * qsemaphore_take() - Take a semaphore.
 * @sem: Pointer to the semaphore initialized by qsemaphore_init().
 * @timeout_ms: Delay until timeout in ms. Use `QOSAL_WAIT_FOREVER` to wait
 * indefinitely.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qsemaphore_take(struct qsemaphore *sem, uint32_t timeout_ms);

/**
 * qsemaphore_give() - Give a semaphore.
 * @sem: Pointer to the semaphore initialized by qsemaphore_init().
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qsemaphore_give(struct qsemaphore *sem);

#ifdef __cplusplus
}
#endif
