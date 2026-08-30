/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "persistent_config.h"

/* TODO: that structure is not yet persistent but stored in SRAM. */
static struct persistent_config_t persistent_config_rram = {
	.low_power_activated = 0,
};

const struct persistent_config_t *persistent_config_get(void)
{
	return (const struct persistent_config_t *)&persistent_config_rram;
}

int persistent_config_set_low_power_activated(uint8_t low_power_activated)
{
	persistent_config_rram.low_power_activated = low_power_activated;
	return 0;
}
