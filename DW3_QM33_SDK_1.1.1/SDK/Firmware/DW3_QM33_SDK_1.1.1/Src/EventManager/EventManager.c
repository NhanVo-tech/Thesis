/**
 * @file      EventManager.c
 *
 * @brief     Event manager functionalities implementation
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <stdint.h>
#include <stddef.h>

#include "EventManager.h"

#include "qmsg_queue.h"

/* Queue depth depends on number of asynchronous events writing to the queue.
 * We should have only one app running at a time we set the depth to 3 to avoid blocking the queue in case of full. */
#define QUEUE_DEPTH 3

static struct qmsg_queue *task_queue_event = NULL; /* Message queue to handle requested application. */
static char task_queue_buffer[sizeof(app_definition_t *) * QUEUE_DEPTH];

void EventManagerInit(void)
{
    task_queue_event = qmsg_queue_init(task_queue_buffer, sizeof(app_definition_t *), QUEUE_DEPTH);
}

bool EventManagerRegisterApp(const app_definition_t **p_app)
{
    /* Add a timeout of 0 as parameters. */
    if (qmsg_queue_put(task_queue_event, p_app) != QERR_SUCCESS)
    {
        return false;
    }
    return true;
}

int EventManagerWaitAppRegistration(const app_definition_t *p_app, uint32_t timeout)
{
    /* Add a timeout as parameter. */
    if (qmsg_queue_get(task_queue_event, (void *)p_app, timeout) == QERR_SUCCESS)
    {
        return 0;
    }

    return -1;
}
