/**
 * @file      HAL_error.h
 *
 * @brief     Interface for HAL_error
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include "deca_error.h"

/**
 * @brief HAL error signaling Red LED if block is set.
 *
 * @param block Blocking function with Red LED flashing.
 *
 * @param err Error code raised.
 */
void error_handler(int block, error_e err);

/**
 * @brief HAL error signaling Red LED if block is set.
 *
 * @return last raised error code.
 */
error_e get_lastErrorCode(void);
