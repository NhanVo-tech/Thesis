/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qpm.h"

#include "qerr.h"

#include <stdbool.h>

void qpm_sleep_state_lock(enum qpm_sleep_state state, uint8_t substate_id)
{
}

void qpm_sleep_state_unlock(enum qpm_sleep_state state, uint8_t substate_id)
{
}

bool qpm_sleep_state_is_active(enum qpm_sleep_state state, uint8_t substate_id)
{
	return false;
}

enum qerr qpm_set_low_power_mode(bool enabled)
{
	return QERR_ENOTSUP;
}

bool qpm_get_low_power_mode(void)
{
	return false;
}

enum qerr qpm_get_min_inactivity_s4(uint32_t *time_ms)
{
	return QERR_ENOTSUP;
}

enum qerr qpm_set_min_inactivity_s4(uint32_t time_ms)
{
	return QERR_ENOTSUP;
}
