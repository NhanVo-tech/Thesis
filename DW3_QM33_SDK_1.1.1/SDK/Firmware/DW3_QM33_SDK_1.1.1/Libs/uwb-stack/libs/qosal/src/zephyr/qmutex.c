/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qmutex.h"

#include <stdlib.h>
#include <zephyr.h>

#define LOG_TAG "qmutex"
#include "qlog.h"

struct qmutex {
	struct k_mutex mutex;
};

#ifndef CONFIG_QOSAL_MAX_MUTEX
#define CONFIG_QOSAL_MAX_MUTEX 1
#endif

#define MEM_SLAB_ALIGNMENT 4

#ifndef K_MEM_SLAB_DEFINE_STATIC
/* To handle zephyr version without the flag */
#define K_MEM_SLAB_DEFINE_STATIC(a, b, c, d) static K_MEM_SLAB_DEFINE(a, b, c, d)
#endif

K_MEM_SLAB_DEFINE_STATIC(qosal_mutex_slab, sizeof(struct qmutex), CONFIG_QOSAL_MAX_MUTEX,
			 MEM_SLAB_ALIGNMENT);

struct qmutex *qmutex_init(void)
{
	struct qmutex *mutex;
	int r = k_mem_slab_alloc(&qosal_mutex_slab, (void **)&mutex, K_NO_WAIT);
	if (r) {
		QLOGE("No mutex available in memory slab");
		return NULL;
	}

	memset(mutex, 0, sizeof(struct qmutex));
	r = k_mutex_init(&mutex->mutex);
	if (r) {
		k_mem_slab_free(&qosal_mutex_slab, (void **)&mutex);
		return NULL;
	}

	return mutex;
}

void qmutex_deinit(struct qmutex *mutex)
{
	if (!mutex)
		return;

	k_mem_slab_free(&qosal_mutex_slab, (void **)&mutex);
}

enum qerr qmutex_lock(struct qmutex *mutex, uint32_t timeout_ms)
{
	int r;
	k_timeout_t timeout;

	if (!mutex)
		return QERR_EINVAL;

	if (timeout_ms == QOSAL_WAIT_FOREVER)
		timeout = K_FOREVER;
	else
		timeout = K_MSEC(timeout_ms);

	r = k_mutex_lock(&mutex->mutex, timeout);
	return qerr_convert_os_to_qerr(r);
}

enum qerr qmutex_unlock(struct qmutex *mutex)
{
	int r;

	if (!mutex)
		return QERR_EINVAL;

	r = k_mutex_unlock(&mutex->mutex);
	return qerr_convert_os_to_qerr(r);
}
