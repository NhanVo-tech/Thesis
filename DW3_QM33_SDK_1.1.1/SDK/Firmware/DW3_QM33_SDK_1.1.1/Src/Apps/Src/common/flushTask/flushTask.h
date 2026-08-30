/**
 * @file      flushTask.h
 *
 * @brief     Interface for flush task
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

void FlushTaskInit(void);
void FlushTask(void *argument);
void FlushTask_reset(void);
void NotifyFlushTask(void);

#ifdef __cplusplus
}
#endif
