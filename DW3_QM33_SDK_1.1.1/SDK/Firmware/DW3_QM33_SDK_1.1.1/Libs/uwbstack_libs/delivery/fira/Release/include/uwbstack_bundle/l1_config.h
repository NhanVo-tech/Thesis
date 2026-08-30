/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include "l1_config_key_access.h"

#include <qerr.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * struct l1_config_platform_ops - Platform specific callback to used by L1 config plugin.
 */
struct l1_config_platform_ops {
	/**
	 * @reset_to_default: Called when configuration is reset to default.
	 *
	 * Return: QERR_SUCCESS or error.
	 */
	enum qerr (*reset_to_default)(void);
};

/**
 * l1_config_init - Initialize L1 config.
 * @platform_ops: Platform specific operations.
 *
 * Initialize the plugin and register platform specific ops, if any.
 * When configuration uses persistent mode, structure from persistent memory is copied to RAM cache.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr l1_config_init(struct l1_config_platform_ops *platform_ops);

/**
 * l1_config_deinit - Deinitialize L1 config.
 */
void l1_config_deinit(void);

#ifdef __cplusplus
}
#endif
