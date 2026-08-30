/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qworkqueue.h"

#include "FreeRTOS.h"
#include "qmalloc.h"
#include "qmsg_queue.h"
#include "qthread.h"
#include "qtime.h"
#include "queue.h"

#define QWORKQUEUE_DEPTH 3
/* TODO: Should be followed by https://qorvo.atlassian.net/browse/UWB-12365 . */
#define QWORKQUEUE_TASK_STACK_SIZE_BYTES 3072

struct qwq_handler {
	qwork_func function;
	void *priv;
};

struct qworkqueue {
	struct qthread *thread; /* Thread holding the work queue. */
	struct qmsg_queue *msg_queue; /* Message queue containing processes to
					 executes. */
	struct qwq_handler handler; /* Current process (and its argumetns) to
				       execute. */
	uint8_t *stack_buffer; /* Workqueue stack buffer. */
};

void qwork_handler(void *arg)
{
	struct qmsg_queue *msg_queue = (struct qmsg_queue *)arg;
	struct qwq_handler handler;

	while (qmsg_queue_get(msg_queue, &handler, QOSAL_WAIT_FOREVER) == QERR_SUCCESS) {
		if (handler.function) {
			handler.function(handler.priv);
		}
	}
}

struct qworkqueue *qworkqueue_init(qwork_func function, void *priv)
{
	struct qworkqueue *workqueue;
	size_t task_size = QWORKQUEUE_TASK_STACK_SIZE_BYTES;

	if (!function)
		return NULL;

	workqueue = qmalloc(sizeof(struct qworkqueue));
	if (!workqueue)
		return NULL;

	workqueue->msg_queue = qmsg_queue_init(NULL, sizeof(struct qwq_handler), QWORKQUEUE_DEPTH);
	if (!workqueue->msg_queue) {
		qfree(workqueue);
		return NULL;
	}

	workqueue->stack_buffer = qmalloc(task_size);
	if (!workqueue->stack_buffer) {
		qmsg_queue_deinit(workqueue->msg_queue);
		qfree(workqueue);
		return NULL;
	}

	workqueue->thread = qthread_create(qwork_handler, workqueue->msg_queue, "qworkqueue",
					   workqueue->stack_buffer, task_size,
					   QTHREAD_PRIORITY_IDLE);
	if (!workqueue->thread) {
		qfree(workqueue->stack_buffer);
		qmsg_queue_deinit(workqueue->msg_queue);
		qfree(workqueue);
		return NULL;
	}

	workqueue->handler.function = function;
	workqueue->handler.priv = priv;

	return workqueue;
}

enum qerr qworkqueue_schedule_work(struct qworkqueue *workqueue)
{
	if (!workqueue)
		return QERR_EINVAL;

	return qmsg_queue_put(workqueue->msg_queue, &workqueue->handler);
}

enum qerr qworkqueue_cancel_work(struct qworkqueue *workqueue)
{
	if (!workqueue)
		return QERR_EINVAL;

	if (qthread_join(workqueue->thread) == QERR_SUCCESS) {
		qthread_delete(workqueue->thread);
		qmsg_queue_deinit(workqueue->msg_queue);
		qfree(workqueue->stack_buffer);
		qfree(workqueue);
	}

	return QERR_SUCCESS;
}
