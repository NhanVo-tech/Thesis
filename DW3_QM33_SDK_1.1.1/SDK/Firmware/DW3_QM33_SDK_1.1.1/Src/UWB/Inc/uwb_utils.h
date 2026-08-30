/**
 * @file      uwb_utils.h
 *
 * @brief     Common functionalities for UWB device.
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include <stdint.h>

/** @brief Enumeration for available AOA support. */
typedef enum
{
    AOA_ERROR,
    AOA_SUPPORTED,
    AOA_NOT_SUPPORTED
} aoa_supported_t;

/**
 * @brief Check if a UWB device supports Angle of Arrival (AoA) functionality.
 *
 * This function checks if a UWB device with the specified device ID supports Angle of Arrival (AoA) functionality.
 *
 * @param device_id The ID of the UWB device to check.
 *
 * @return The status of AoA functionality for the specified device:
 *         - `AOA_SUPPORTED`: AoA functionality is enabled for the device.
 *         - `AOA_NOT_SUPPORTED`: AoA functionality is disabled for the device.
 *         - `AOA_ERROR`: Device ID is not recognized or an error occurred.
 */
aoa_supported_t uwb_device_is_aoa(uint32_t device_id);
