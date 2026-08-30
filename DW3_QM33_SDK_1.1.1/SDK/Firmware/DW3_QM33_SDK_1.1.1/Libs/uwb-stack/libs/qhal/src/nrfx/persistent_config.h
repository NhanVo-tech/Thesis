/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <stdint.h>

/**
 * struct persistent_config_t - Configuration stored in persistent memory.
 * @low_power_activated: true if Low Power mode is activated.
 */
struct persistent_config_t {
	uint8_t low_power_activated;
};

/**
 * persistent_config_set_low_power_activated() - Set Low Power activation mode
 * in persistent configuration.
 *
 * Return: 0 on success or negative error.
 */
int persistent_config_set_low_power_activated(uint8_t low_power_activated);

/**
 * persistent_config_get() - Get Persistent configuration.
 *
 * Return: persistent configuration structure.
 */
const struct persistent_config_t *persistent_config_get(void);
