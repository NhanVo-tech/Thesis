/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include "qerr.h"
#include "qtime.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * struct qmutex: QOSAL Mutex (opaque).
 */
struct qmutex;

/**
 * qmutex_init() - Initialize a mutex.
 *
 * Return: Pointer to the initialized mutex.
 */
struct qmutex *qmutex_init(void);

/**
 * qmutex_deinit() - De-initialize a mutex.
 * @mutex: Pointer to the mutex initialized by qmutex_init().
 */
void qmutex_deinit(struct qmutex *mutex);

/**
 * qmutex_lock() - Lock a mutex.
 * @mutex: Pointer to the mutex initialized by qmutex_init().
 * @timeout_ms: Delay until timeout in ms. Use `QOSAL_WAIT_FOREVER` to wait
 * indefinitely.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qmutex_lock(struct qmutex *mutex, uint32_t timeout_ms);

/**
 * qmutex_unlock() - Unlock a mutex.
 * @mutex: Pointer to the mutex initialized by qmutex_init().
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qmutex_unlock(struct qmutex *mutex);

#ifdef __cplusplus
}
#endif
