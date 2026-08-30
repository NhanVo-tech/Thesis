/**
 * @file      appConfig.h
 *
 * @brief     Interface for configuration save/load/restore
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "deca_error.h"

#include <stdbool.h>

/** @brief Copies parameters from NVM section to RAM structure. */
void load_bssConfig(bool initStatus);

/** @brief   copies parameters from default RAM section to RAM structure. */
void restore_bssConfig(bool initStatus);

/**
 * @brief   saves parameters from listener, fira and uart and stores them in NVM.
 *
 * @return  _NO_ERR for success and error_e code otherwise
 */
error_e save_bssConfig(void);

#ifdef __cplusplus
}
#endif
