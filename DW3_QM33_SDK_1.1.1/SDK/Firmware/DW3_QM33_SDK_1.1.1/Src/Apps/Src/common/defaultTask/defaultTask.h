/**
 * @file      defaultTask.h
 *
 * @brief     Interface for default task
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

void StartDefaultTask(void *argument);
void DefaultTaskInit(void);
void DefaultTaskTimeoutHook(void);
