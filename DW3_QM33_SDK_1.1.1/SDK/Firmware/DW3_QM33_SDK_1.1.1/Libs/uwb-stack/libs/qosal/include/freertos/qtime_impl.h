/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#define QTIME_GET_UPTIME_TICKS() xTaskGetTickCount()

#define QTIME_GET_STRING_TICKS_PER_S() STRINGIFY(configTICK_RATE_HZ)
