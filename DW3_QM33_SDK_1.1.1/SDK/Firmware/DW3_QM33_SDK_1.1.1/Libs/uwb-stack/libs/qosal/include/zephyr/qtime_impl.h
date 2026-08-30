/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <kernel.h>

#define QTIME_GET_UPTIME_TICKS() k_uptime_ticks()

#define QTIME_GET_STRING_TICKS_PER_S() STRINGIFY(Z_HZ_ticks)
