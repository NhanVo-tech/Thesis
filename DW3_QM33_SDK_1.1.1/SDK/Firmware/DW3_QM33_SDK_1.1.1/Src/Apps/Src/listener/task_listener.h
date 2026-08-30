/**
 * @file      task_listener.h
 *
 * @brief     Interface for listener task
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void listener_task_notify(void);
bool listener_task_started(void);


#ifdef __cplusplus
}
#endif
