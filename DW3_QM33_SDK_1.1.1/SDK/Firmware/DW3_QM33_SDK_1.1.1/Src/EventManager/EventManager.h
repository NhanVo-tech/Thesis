/**
 * @file      EventManager.h
 *
 * @brief     Interface for Event Manager
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "app.h"

void EventManagerInit(void);
bool EventManagerRegisterApp(const app_definition_t **p_app);
int EventManagerWaitAppRegistration(const app_definition_t *p_app, uint32_t timeout);
