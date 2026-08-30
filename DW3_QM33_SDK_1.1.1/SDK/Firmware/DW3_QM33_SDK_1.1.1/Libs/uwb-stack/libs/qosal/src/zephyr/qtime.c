/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qtime.h"

#include <kernel.h>

int64_t qtime_get_uptime_us(void)
{
	return k_ticks_to_us_floor64(k_uptime_ticks());
}

void qtime_msleep(int ms)
{
	k_msleep(ms);
}

void qtime_usleep(int us)
{
	k_usleep(us);
}

uint32_t qtime_get_sys_freq_hz()
{
	return sys_clock_hw_cycles_per_sec();
}
