/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qsemaphore.h"

#include "FreeRTOS.h"
#include "qirq.h"
#include "qmalloc.h"
#include "qprivate.h"
#include "semphr.h"

struct qsemaphore {
	SemaphoreHandle_t sem;
};

struct qsemaphore *qsemaphore_init(uint32_t init_count, uint32_t max_count)
{
	struct qsemaphore *semaphore = qmalloc(sizeof(struct qsemaphore));
	if (!semaphore)
		return NULL;

	semaphore->sem = xSemaphoreCreateCounting((UBaseType_t)max_count, (UBaseType_t)init_count);
	if (semaphore->sem == NULL) {
		qfree(semaphore);
		return NULL;
	}

	return semaphore;
}

void qsemaphore_deinit(struct qsemaphore *sem)
{
	if (!sem)
		return;

	vSemaphoreDelete(sem->sem);
	qfree(sem);
}

enum qerr qsemaphore_take(struct qsemaphore *sem, uint32_t timeout_ms)
{
	BaseType_t ret_val = pdFALSE;

	if (!sem)
		return QERR_EINVAL;

	if (qprivate_is_in_isr()) {
		portBASE_TYPE task_woken = pdFALSE;

		ret_val = xSemaphoreTakeFromISR(sem->sem, &task_woken);
	} else {
		TickType_t ticks = 0;

		if (timeout_ms == QOSAL_WAIT_FOREVER)
			ticks = portMAX_DELAY;
		else if (timeout_ms != 0)
			ticks = pdMS_TO_TICKS(timeout_ms);

		ret_val = xSemaphoreTake(sem->sem, ticks);
	}

	if (!ret_val)
		return QERR_EIO;

	return QERR_SUCCESS;
}

enum qerr qsemaphore_give(struct qsemaphore *sem)
{
	BaseType_t ret_val = pdFALSE;

	if (!sem)
		return QERR_EINVAL;

	if (qprivate_is_in_isr()) {
		portBASE_TYPE task_woken = pdFALSE;

		ret_val = xSemaphoreTakeFromISR(sem->sem, &task_woken);
	} else {
		ret_val = xSemaphoreGive(sem->sem);
	}

	if (!ret_val)
		return QERR_EIO;

	return QERR_SUCCESS;
}
