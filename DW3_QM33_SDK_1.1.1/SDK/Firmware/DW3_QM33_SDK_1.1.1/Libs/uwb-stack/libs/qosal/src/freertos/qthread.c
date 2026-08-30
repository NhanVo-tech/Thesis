/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qthread.h"

#include "FreeRTOS.h"
#include "qmalloc.h"
#include "qsignal.h"
#include "task.h"

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so
the application must provide an implementation of
vApplicationGetTimerTaskMemory() to provide the memory that is used by the Timer
service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppx_timer_task_tcb_buffer,
				    StackType_t **ppx_timer_task_stack_buffer,
				    uint32_t *pul_timer_task_stack_size)
{
	/* If the buffers to be provided to the Timer task are declared inside
	this function then they must be declared static - otherwise they will be
	allocated on the stack and so not exists after this function exits. */
	static StaticTask_t timer_task_tcb;
	static StackType_t ux_timer_task_stack[configTIMER_TASK_STACK_DEPTH];

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppx_timer_task_tcb_buffer = &timer_task_tcb;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppx_timer_task_stack_buffer = ux_timer_task_stack;

	/* Pass out the size of the array pointed to by
	*ppx_timer_task_stack_buffer. Note that, as the array is necessarily of
	type StackType_t, configTIMER_TASK_STACK_DEPTH is specified in words,
	not bytes. */
	*pul_timer_task_stack_size = configTIMER_TASK_STACK_DEPTH;
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppx_idle_task_tcb_buffer,
				   StackType_t **ppx_idle_task_stack_buffer,
				   uint32_t *pul_idle_task_stack_size)
{
	/* If the buffers to be provided to the Idle task are declared inside
	this function then they must be declared static - otherwise they will be
	allocated on the stack and so not exists after this function exits. */
	static StaticTask_t idle_task_tcb;
	static StackType_t ux_idle_task_stack[configMINIMAL_STACK_SIZE];

	/* Pass out a pointer to the StaticTask_t structure in which the Idle
	task's state will be stored. */
	*ppx_idle_task_tcb_buffer = &idle_task_tcb;

	/* Pass out the array that will be used as the Idle task's stack. */
	*ppx_idle_task_stack_buffer = ux_idle_task_stack;

	/* Pass out the size of the array pointed to by
	*ppx_idle_task_stack_buffer. Note that, as the array is necessarily of
	type StackType_t, configMINIMAL_STACK_SIZE is specified in words, not
	bytes. */
	*pul_idle_task_stack_size = configMINIMAL_STACK_SIZE;
}

struct qthread {
	TaskHandle_t handler;
	StaticTask_t task_buffer;
};

struct qthread *qthread_create(qthread_func thread_function, void *arg, const char *name,
			       void *stack_buffer, uint32_t stack_size, enum qthread_priority prio)
{
	struct qthread *thread;
	portBASE_TYPE new_prio;

	if (!thread_function || !name)
		return NULL;

	if (!stack_buffer)
		return NULL;

	if (prio >= QTHREAD_PRIORITY_MAX)
		return NULL;

	thread = (struct qthread *)qmalloc(sizeof(struct qthread));
	if (!thread)
		return NULL;
	thread->handler = NULL;

	new_prio = (QTHREAD_PRIORITY_MAX - 1) - prio;

	/* If stack buffer is provided by user. */
	thread->handler = xTaskCreateStatic(thread_function, name,
					    QALIGN(stack_size, sizeof(int)) / sizeof(int), arg,
					    new_prio, stack_buffer, &thread->task_buffer);

	if (!thread->handler) {
		qfree(thread);
		return NULL;
	}

	return thread;
}

enum qerr qthread_join(struct qthread *thread)
{
	do {
		if (eTaskGetState(thread->handler) != eRunning)
			break;

		vTaskDelay(portTICK_PERIOD_MS);
	} while (1);

	return QERR_SUCCESS;
}

enum qerr qthread_delete(struct qthread *thread)
{
	TaskHandle_t handler = thread->handler;

	qfree(thread);
	vTaskDelete(handler);

	return QERR_SUCCESS;
}

enum qerr qthread_yield(void)
{
	taskYIELD();

	return QERR_SUCCESS;
}
