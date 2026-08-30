/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qsemaphore.h"

#include "qmalloc.h"

#include <stdlib.h>
#include <zephyr.h>

#define QLOG_CURRENT_LEVEL QLOG_LEVEL_ERR
#define LOG_TAG "qsemaphore"
#include "qlog.h"

struct qsemaphore {
	struct k_sem sem;
};

struct qsemaphore *qsemaphore_init(uint32_t init_count, uint32_t max_count)
{
	int r;

	/* Memory slab cannot be used as the number of semaphores needed may
	 * vary. */
	struct qsemaphore *semaphore = qmalloc(sizeof(struct qsemaphore));
	if (!semaphore)
		return NULL;

	r = k_sem_init(&semaphore->sem, init_count, max_count);

	if (r) {
		qfree(semaphore);
		return NULL;
	}

	return semaphore;
}

void qsemaphore_deinit(struct qsemaphore *sem)
{
	if (!sem)
		return;

	k_sem_reset(&sem->sem);
	qfree(sem);
}

enum qerr qsemaphore_take(struct qsemaphore *sem, uint32_t timeout_ms)
{
	int r;
	k_timeout_t timeout;

	if (!sem)
		return QERR_EINVAL;

	if (timeout_ms == QOSAL_WAIT_FOREVER)
		timeout = K_FOREVER;
	else
		timeout = K_MSEC(timeout_ms);

	r = k_sem_take(&sem->sem, timeout);
	return qerr_convert_os_to_qerr(r);
}

enum qerr qsemaphore_give(struct qsemaphore *sem)
{
	if (!sem)
		return QERR_EINVAL;

	k_sem_give(&sem->sem);

	return QERR_SUCCESS;
}
