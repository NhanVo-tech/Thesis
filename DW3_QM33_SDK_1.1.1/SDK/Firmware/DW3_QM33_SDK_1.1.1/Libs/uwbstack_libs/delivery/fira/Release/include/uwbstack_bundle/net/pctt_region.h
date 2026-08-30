/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "qerr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * pctt_region_init() - Initialize PCTT region.
 * Return: QERR_SUCCESS or error
 */
enum qerr pctt_region_init(void);

/**
 * pctt_region_exit() - Leave PCTT region.
 */
void pctt_region_exit(void);

#ifdef __cplusplus
}
#endif
