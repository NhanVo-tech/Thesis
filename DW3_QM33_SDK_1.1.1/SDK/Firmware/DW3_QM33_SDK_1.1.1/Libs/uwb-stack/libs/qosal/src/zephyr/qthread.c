/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qthread.h"

#include <stdlib.h>
#include <zephyr.h>

#define LOG_TAG "qthread"
#include "qlog.h"

struct qthread {
	struct k_thread thread;
};

#ifndef CONFIG_QOSAL_MAX_THREAD
#define CONFIG_QOSAL_MAX_THREAD 2
#endif

#define MEM_SLAB_ALIGNMENT 4

#ifndef K_MEM_SLAB_DEFINE_STATIC
/* To handle zephyr version without the flag */
#define K_MEM_SLAB_DEFINE_STATIC(a, b, c, d) static K_MEM_SLAB_DEFINE(a, b, c, d)
#endif

K_MEM_SLAB_DEFINE_STATIC(qosal_thread_slab, sizeof(struct qthread), CONFIG_QOSAL_MAX_THREAD,
			 MEM_SLAB_ALIGNMENT);

/* Wrapper function for Zephyr thread: p1 gets QThread entry point
 * and p2 gets QThread private argument. */
static void zephyr_thread_entry(void *p1, void *p2, void *p3)
{
	qthread_func func = p1;
	(void)p3;

	func(p2);
}

struct qthread *qthread_create(qthread_func thread, void *arg, const char *name, void *stack,
			       uint32_t stack_size, enum qthread_priority prio)
{
	struct qthread *th;
	int r;

	if (!thread || !name)
		return NULL;

	if (!stack) {
		QLOGE("Dynamic allocation of thread stack is not supported");
		return NULL;
	}

	if (prio >= QTHREAD_PRIORITY_MAX)
		return NULL;

	r = k_mem_slab_alloc(&qosal_thread_slab, (void **)&th, K_NO_WAIT);
	if (r) {
		QLOGE("No thread available in memory slab");
		return NULL;
	}

	memset(th, 0, sizeof(struct qthread));
	k_thread_create(&th->thread, stack, stack_size, zephyr_thread_entry, thread, arg, NULL,
			prio, 0, K_NO_WAIT);

	k_thread_name_set(&th->thread, name);

	return th;
}

enum qerr qthread_join(struct qthread *thread)
{
	int r;

	if (!thread)
		return QERR_EINVAL;

	r = k_thread_join(&thread->thread, K_FOREVER);
	if (r)
		return qerr_convert_os_to_qerr(r);

	k_mem_slab_free(&qosal_thread_slab, (void **)&thread);
	return QERR_SUCCESS;
}

enum qerr qthread_delete(struct qthread *thread)
{
	if (!thread)
		return QERR_EINVAL;

	k_thread_abort(&thread->thread);
	k_mem_slab_free(&qosal_thread_slab, (void **)&thread);
	return QERR_SUCCESS;
}
