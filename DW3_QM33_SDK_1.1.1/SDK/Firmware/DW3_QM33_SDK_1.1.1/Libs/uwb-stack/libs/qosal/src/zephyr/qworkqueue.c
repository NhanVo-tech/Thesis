/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qworkqueue.h"

#include "qmalloc.h"

#include <stdlib.h>
#include <zephyr.h>

#if CONFIG_SYSTEM_WORKQUEUE_PRIORITY <= CONFIG_MAIN_THREAD_PRIORITY
/* workqueue are used to offload time consuming task that are not time critical
 * like printing logs */
#warning CONFIG_SYSTEM_WORKQUEUE_PRIORITY need to have a lower priority than CONFIG_MAIN_THREAD_PRIORITY
#endif

struct qworkqueue {
	struct k_work work;
	qwork_func handler;
	void *priv;
};

void z_work_handler(struct k_work *work)
{
	struct qworkqueue *workqueue = CONTAINER_OF(work, struct qworkqueue, work);

	workqueue->handler(workqueue->priv);
}

struct qworkqueue *qworkqueue_init(qwork_func handler, void *priv)
{
	if (!handler)
		return NULL;

	/* Memory slab cannot be used as the number of workqueues needed may
	 * vary. */
	struct qworkqueue *workqueue = qmalloc(sizeof(struct qworkqueue));
	if (!workqueue)
		return NULL;

	k_work_init(&workqueue->work, z_work_handler);
	workqueue->handler = handler;
	workqueue->priv = priv;

	return workqueue;
}

enum qerr qworkqueue_schedule_work(struct qworkqueue *workqueue)
{
	int r;

	if (!workqueue)
		return QERR_EINVAL;

	r = k_work_submit(&workqueue->work);
	return qerr_convert_os_to_qerr(r);
}

enum qerr qworkqueue_cancel_work(struct qworkqueue *workqueue)
{
	bool r;

	if (!workqueue)
		return QERR_EINVAL;

	struct k_work_sync work_sync;
	r = k_work_cancel_sync(&workqueue->work, &work_sync);
	qfree(workqueue);

	/* If true, return QERR_BUSY as work is pending. */
	if (r)
		return QERR_EBUSY;

	return QERR_SUCCESS;
}
