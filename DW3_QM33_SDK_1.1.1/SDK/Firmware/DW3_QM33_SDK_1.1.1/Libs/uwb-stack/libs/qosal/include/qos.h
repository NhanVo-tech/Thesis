/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qerr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Starts the operating system scheduler.
 *
 * @return QERR_SUCCESS or error.
 */
enum qerr qos_start(void);

#ifdef __cplusplus
}
#endif
