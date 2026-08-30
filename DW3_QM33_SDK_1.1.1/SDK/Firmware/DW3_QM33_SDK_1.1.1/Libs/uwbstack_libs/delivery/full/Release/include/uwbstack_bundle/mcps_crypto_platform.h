/*
 * SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <qerr.h>
#include <stdint.h>

/**
 * mcps_crypto_init() - Initialize crypto.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr mcps_crypto_init(void);

/**
 * mcps_crypto_deinit() - Deinitialize crypto.
 */
void mcps_crypto_deinit(void);
