/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qmsg_queue.h"

#include "FreeRTOS.h"
#include "qirq.h"
#include "qmalloc.h"
#include "qprivate.h"
#include "queue.h"
#include "task.h"

#include <stdbool.h>

struct qmsg_queue {
	QueueHandle_t queue; /* Message Queue Handler. */
	StaticQueue_t static_queue; /* Hold the queue data structure. */
	char *msg_queue_buffer; /* Message queue buffer in case of dynamic
				   allocation. */
	bool allocated; /* true if allocation buffer made by inside qosal. false
			   otherwise. */
};

struct qmsg_queue *qmsg_queue_init(char *msg_queue_buffer, uint32_t item_size, uint32_t max_item)
{
	struct qmsg_queue *msg_queue;
	msg_queue = qmalloc(sizeof(struct qmsg_queue));
	if (!msg_queue)
		return NULL;

	/* Check if buffer shall be allocated automatically by FreeRTOS. */
	msg_queue->msg_queue_buffer = msg_queue_buffer;
	msg_queue->allocated = false;
	if (msg_queue_buffer == NULL) {
		msg_queue->msg_queue_buffer = qmalloc(item_size * max_item);

		if (!msg_queue->msg_queue_buffer) {
			qfree(msg_queue);
			return NULL;
		}
		msg_queue->allocated = true;
	}

	msg_queue->queue = xQueueCreateStatic(max_item, item_size,
					      (uint8_t *)msg_queue->msg_queue_buffer,
					      &msg_queue->static_queue);

	if (msg_queue->queue == NULL) {
		if (msg_queue->allocated)
			qfree(msg_queue->msg_queue_buffer);

		qfree(msg_queue);
		return NULL;
	}

	return msg_queue;
}

void qmsg_queue_deinit(struct qmsg_queue *msg_queue)
{
	if (!msg_queue)
		return;

	if (msg_queue->allocated)
		qfree(msg_queue->msg_queue_buffer);

	vQueueDelete(msg_queue->queue);

	qfree(msg_queue);
}

enum qerr qmsg_queue_put(struct qmsg_queue *msg_queue, const void *item)
{
	BaseType_t ret_val = pdFALSE;

	if (qprivate_is_in_isr()) {
		portBASE_TYPE high_priority_task_woken = pdFALSE;

		ret_val = xQueueSendFromISR(msg_queue->queue, item, &high_priority_task_woken);

		if (high_priority_task_woken == pdTRUE)
			portYIELD_FROM_ISR(high_priority_task_woken);

	} else {
		ret_val = xQueueSend(msg_queue->queue, item, 0);
	}

	if (!ret_val)
		return QERR_EIO;

	return QERR_SUCCESS;
}

enum qerr qmsg_queue_get(struct qmsg_queue *msg_queue, void *item, uint32_t timeout_ms)
{
	BaseType_t ret_val = pdFALSE;

	if (!msg_queue || !item)
		return QERR_EINVAL;

	if (qprivate_is_in_isr()) {
		portBASE_TYPE task_woken = pdFALSE;

		ret_val = xQueueReceiveFromISR(msg_queue->queue, item, &task_woken);
	} else {
		TickType_t ticks = 0;

		if (timeout_ms == QOSAL_WAIT_FOREVER)
			ticks = portMAX_DELAY;
		else if (timeout_ms != 0)
			ticks = pdMS_TO_TICKS(timeout_ms);

		ret_val = xQueueReceive(msg_queue->queue, item, ticks);
	}

	if (!ret_val)
		return QERR_EIO;

	return QERR_SUCCESS;
}
