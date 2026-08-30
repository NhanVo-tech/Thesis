/**
 * @file      defaultTask.c
 *
 * @brief     Default Task
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "defaultTask.h"

#include "EventManager.h"
#include "qthread.h"
#include "int_priority.h"
#include "HAL_error.h"
#include "HAL_watchdog.h"
#include "qmalloc.h"
#ifdef USB_ENABLE
#include "HAL_usb.h"
#endif

/* Define The timeout for default task wait app registration
 * Message Queue Timeout must be smaller than watchdog period. */
#define MESSAGE_QUEUE_TIMEOUT_MS 200

/* Worst case is RESPF followed by STOP command on a Nordic platform. */
#define DEFAULT_TASK_STACK_SIZE_BYTES 4300
/* Default Task handler */
static struct qthread *default_task = NULL;

__attribute__((weak)) const app_definition_t *DefaultTaskHookEvent()
{
    return AppGetDefaultEvent();
}

__attribute__((weak)) void DefaultTaskTimeoutHook(void)
{
}

/**
 * @brief DefaultTask task execution.
 * Manage TOP level application start and stop.
 */
void StartDefaultTask(void *argument)
{
    const app_definition_t *queue_message;
    const app_definition_t *default_app = DefaultTaskHookEvent();

    if (default_app != NULL)
    {
        EventManagerRegisterApp(&default_app);
    }
    /* Infinite loop. */
    while (1)
    {
        Watchdog.refresh();
        /* Contract: registered App should point to a valid application from the flash. */
        if (EventManagerWaitAppRegistration((app_definition_t *)&queue_message, MESSAGE_QUEUE_TIMEOUT_MS) != -1)
        {
            /* clean the resources of the current executed task. */
            if (AppGet()->terminate != NULL)
            {
                AppGet()->terminate();
            }

            /* Set current running application to NULL. */
            AppSet(NULL);
            /* Force switch of context. */
            qthread_yield();
            /* Start appropriate RTOS top-level application:
             * Contract: registered App should point to a valid application from the flash. */
            AppSet(queue_message);
            if (queue_message->helper != NULL)
            {
                queue_message->helper(NULL);
            }
        }
        else
        {
            DefaultTaskTimeoutHook();
        }
#ifdef USB_ENABLE
        Usb.update();
#endif
        qthread_yield();
    }
}

/**
 * @brief Creation of Default task.
 * The Default task is responsible for starting & stopping of TOP Level applications.
 */
void DefaultTaskInit(void)
{
    /* Create Default Task */
    size_t task_size = DEFAULT_TASK_STACK_SIZE_BYTES;
    uint8_t *task_stack = qmalloc(task_size);

    default_task = qthread_create(StartDefaultTask, NULL, "Default", task_stack, task_size, PRIO_StartDefaultTask);
    if (!default_task)
    {
        error_handler(1, _ERR_Create_Task_Bad);
    }
}
