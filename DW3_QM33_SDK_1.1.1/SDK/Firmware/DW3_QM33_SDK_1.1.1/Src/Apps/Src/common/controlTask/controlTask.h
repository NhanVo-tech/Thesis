/**
 * @file      controlTask.h
 *
 * @brief     Interface for control task
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

#include "circular_buffer.h"


void ControlTaskInit(void);
void NotifyControlTaskData(struct cc_buff *buf);
void NotifyControlTaskStopApp(void);

#ifdef __cplusplus
}
#endif
