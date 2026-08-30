/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qprofiling.h"

#include <linker/linker-defs.h>
#include <malloc.h>
#define LOG_TAG "qprofstat"
#include "qlog.h"

#ifdef CONFIG_QOSAL_PROFILING_STATS
#include <stdio.h>
#include <zephyr/debug/stack.h>
#include <zephyr/kernel.h>

void qmemstat(void)
{
	struct mallinfo minfo = mallinfo();
	QLOGI("dynamic memory allocated peak: %u bytes", minfo.arena);
}

/*
 * z_stack_space_get is defined in kernel_internal.h which is unavailable
 * outside of the kernel library. Therefore it is added as external
 */
extern int z_stack_space_get(const uint8_t *stack_start, size_t size, size_t *unused_ptr);

/* Get z_interrupt_stacks array from zephyr */
extern K_KERNEL_STACK_ARRAY_DEFINE(z_interrupt_stacks, CONFIG_MP_NUM_CPUS, CONFIG_ISR_STACK_SIZE);

static void isr_stacks(void)
{
	for (int i = 0; i < CONFIG_MP_NUM_CPUS; i++) {
		const uint8_t *buf = Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[i]);
		size_t size = K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[i]);
		size_t unused = 0;

		if (z_stack_space_get(buf, size, &unused) == 0) {
			QLOGI("thread ISR%d: stack usage %zu/%zu bytes", i, size - unused, size);
		}
	}
}

int qstackstat_count_get(void)
{
	struct k_thread *thread;
	int thread_count = 0;

	/* Each thread has its own stack. */
	for (thread = _kernel.threads; thread; thread = thread->next_thread)
		thread_count++;
	/* Add one stack per interrupt handler (one per CPU) to the count. */
	return thread_count + CONFIG_MP_NUM_CPUS;
}

int qstackstat_get(struct qstackstats *stats, int stack_count)
{
	struct qstackstats *stack = stats;
	struct k_thread *thread;

	/* Fill in the info for the stack of each interrupt handler (one per
	 * CPU). */
	for (int i = 0; i < CONFIG_MP_NUM_CPUS && (stack - stats) < stack_count; i++) {
		const uint8_t *buf = Z_KERNEL_STACK_BUFFER(z_interrupt_stacks[i]);
		size_t size = K_KERNEL_STACK_SIZEOF(z_interrupt_stacks[i]);
		size_t unused;

#if QOSAL_THREAD_MAX_NAME_LEN > 0
		snprintf(stack->thread_name, QOSAL_THREAD_MAX_NAME_LEN - 1, "ISR%d", i);
		stack->thread_name[QOSAL_THREAD_MAX_NAME_LEN - 1] = '\0';
#endif
#ifdef CONFIG_CPU_CORTEX_M
		if (arch_curr_cpu()->id == i)
			stack->stack_used = (uint32_t)buf + size - __get_MSP();
		else
			stack->stack_used = -1;
#else
		stack->stack_used = -1;
#endif
		if (z_stack_space_get(buf, size, &unused) == 0)
			stack->stack_peak = size - unused;
		else
			stack->stack_peak = -1;
		stack->stack_size = size;

		stack++;
	}

	/* Fill in the info for the stack of each thread. */
	for (thread = _kernel.threads; thread && (stack - stats) < stack_count;
	     thread = thread->next_thread) {
		size_t unused;

#if QOSAL_THREAD_MAX_NAME_LEN > 0
		const char *name = k_thread_name_get((k_tid_t)thread);
		if (name) {
			strncpy(stack->thread_name, name, CONFIG_THREAD_MAX_NAME_LEN - 1);
			stack->thread_name[CONFIG_THREAD_MAX_NAME_LEN - 1] = '\0';
		} else {
			stack->thread_name[0] = '\0';
		}
#endif
#ifdef CONFIG_CPU_CORTEX_M
		stack->stack_used = thread->stack_info.start + thread->stack_info.size -
				    thread->callee_saved.psp;
#else
		stack->stack_used = -1;
#endif
		if (k_thread_stack_space_get(thread, &unused) == 0)
			stack->stack_peak = thread->stack_info.size - unused;
		else
			stack->stack_peak = -1;
		stack->stack_size = thread->stack_info.size;

		stack++;
	}

	return stack - stats;
}

void qstackstat(void)
{
	struct k_thread *thread;

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		size_t space;
		const char *name;
		if (k_thread_stack_space_get(thread, &space))
			continue;
		/* Get the thread name if it is defined (if CONFIG_THREAD_NAME
		 * is set)*/
		name = k_thread_name_get((k_tid_t)thread);
		if (!name || name[0] == '\0') {
			QLOGI("thread %p: stack usage %zu/%zu bytes", thread->entry.pEntry,
			      thread->stack_info.size - space, thread->stack_info.size);
		} else {
			QLOGI("thread %s: stack usage %zu/%zu bytes", name,
			      thread->stack_info.size - space, thread->stack_info.size);
		}
	}

	isr_stacks();
}
#else
void qmemstat(void)
{
}
int qstackstat_count_get()
{
	return 0;
}
int qstackstat_get(struct qstackstats *stats, int stack_count)
{
	return 0;
}
void qstackstat(void)
{
}
#endif

void qmemstat_get(struct qmemstats *stats)
{
	struct mallinfo minfo = mallinfo();

	stats->static_size = (uintptr_t)&_image_ram_end - (uintptr_t)&_image_ram_start;
	stats->heap_used = minfo.uordblks;
	stats->heap_peak = minfo.arena;
	stats->heap_size = (uintptr_t)&__kernel_ram_end - (uintptr_t)&_image_ram_end;
}

void qprofstat(void)
{
	qmemstat();
	qstackstat();
}
