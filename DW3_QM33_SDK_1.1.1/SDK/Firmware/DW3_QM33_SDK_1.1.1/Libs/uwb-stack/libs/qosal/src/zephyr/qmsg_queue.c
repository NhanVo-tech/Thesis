/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qmsg_queue.h"

#include <stdbool.h>
#include <stdlib.h>
#include <zephyr.h>

#define LOG_TAG "qmsg_queue"
#include "qlog.h"

struct qmsg_queue {
	struct k_msgq msgq;
	bool allocated;
};

#ifndef CONFIG_QOSAL_MAX_MSG_QUEUE
#define CONFIG_QOSAL_MAX_MSG_QUEUE 1
#endif

#define MEM_SLAB_ALIGNMENT 4

#ifndef K_MEM_SLAB_DEFINE_STATIC
/* To handle zephyr version without the flag */
#define K_MEM_SLAB_DEFINE_STATIC(a, b, c, d) static K_MEM_SLAB_DEFINE(a, b, c, d)
#endif

K_MEM_SLAB_DEFINE_STATIC(qosal_msg_queue_slab, sizeof(struct qmsg_queue),
			 CONFIG_QOSAL_MAX_MSG_QUEUE, MEM_SLAB_ALIGNMENT);

struct qmsg_queue *qmsg_queue_init(char *msg_queue_buffer, uint32_t item_size, uint32_t max_item)
{
	struct qmsg_queue *msg_queue;
	int r = k_mem_slab_alloc(&qosal_msg_queue_slab, (void **)&msg_queue, K_NO_WAIT);
	if (r) {
		QLOGE("No message queue available in memory slab");
		return NULL;
	}
	memset(msg_queue, 0, sizeof(struct qmsg_queue));

	/* Automatically allocated buffer if not provided. */
	if (msg_queue_buffer == NULL) {
		r = k_msgq_alloc_init(&msg_queue->msgq, item_size, max_item);
		if (r) {
			k_mem_slab_free(&qosal_msg_queue_slab, (void **)&msg_queue);
			return NULL;
		}
		msg_queue->allocated = true;
	} else {
		k_msgq_init(&msg_queue->msgq, msg_queue_buffer, item_size, max_item);
		msg_queue->allocated = false;
	}
	return msg_queue;
}

void qmsg_queue_deinit(struct qmsg_queue *msg_queue)
{
	if (!msg_queue)
		return;

	if (msg_queue->allocated) {
		k_msgq_cleanup(&msg_queue->msgq);
	}

	k_mem_slab_free(&qosal_msg_queue_slab, (void **)&msg_queue);
}

enum qerr qmsg_queue_put(struct qmsg_queue *msg_queue, const void *item)
{
	int r;

	if (!msg_queue || !item)
		return QERR_EINVAL;

	r = k_msgq_put(&msg_queue->msgq, item, K_NO_WAIT);
	return qerr_convert_os_to_qerr(r);
}

enum qerr qmsg_queue_get(struct qmsg_queue *msg_queue, void *item, uint32_t timeout_ms)
{
	int r;
	k_timeout_t timeout;

	if (!msg_queue || !item)
		return QERR_EINVAL;

	if (timeout_ms == QOSAL_WAIT_FOREVER)
		timeout = K_FOREVER;
	else
		timeout = K_MSEC(timeout_ms);

	r = k_msgq_get(&msg_queue->msgq, item, timeout);
	return qerr_convert_os_to_qerr(r);
}
