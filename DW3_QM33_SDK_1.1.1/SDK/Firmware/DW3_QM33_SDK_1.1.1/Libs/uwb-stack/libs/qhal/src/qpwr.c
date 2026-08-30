/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qpwr.h"

#include <persistent_config.h>
#include <qerr.h>

#ifdef CONFIG_PM
#include <qm_pm.h>
#endif

void qpwr_enable_lpm(void)
{
	persistent_config_set_low_power_activated(1);
}

void qpwr_disable_lpm(void)
{
	persistent_config_set_low_power_activated(0);
}

bool qpwr_is_lpm_enabled(void)
{
	const struct persistent_config_t *cfg = persistent_config_get();
	return !!cfg->low_power_activated;
}

#ifdef CONFIG_PM
enum qerr qpwr_set_min_inactivity_s4(uint32_t time_ms)
{
	int rc = qm_pm_set_s4_delay_time_ms(time_ms);
	if (rc)
		return QERR_ENOTSUP;

	return QERR_SUCCESS;
}

enum qerr qpwr_get_min_inactivity_s4(uint32_t *time_ms)
{
	if (time_ms == NULL)
		return QERR_EINVAL;

	*time_ms = qm_pm_get_s4_delay_time_ms();
	return QERR_SUCCESS;
}
#else
enum qerr qpwr_set_min_inactivity_s4(uint32_t time_ms)
{
	return QERR_ENOTSUP;
}

enum qerr qpwr_get_min_inactivity_s4(uint32_t *time_ms)
{
	return QERR_ENOTSUP;
}
#endif
