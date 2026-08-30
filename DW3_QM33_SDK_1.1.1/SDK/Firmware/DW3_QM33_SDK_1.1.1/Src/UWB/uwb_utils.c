/**
 * @file      uwb_utils.c
 *
 * @brief     Common functionalities for UWB device.
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "uwb_utils.h"

#include "deca_device_api.h"

aoa_supported_t uwb_device_is_aoa(uint32_t device_id)
{
    switch (device_id)
    {
        case DWT_DW3000_PDOA_DEV_ID:
        case DWT_DW3720_PDOA_DEV_ID:
            return AOA_SUPPORTED;
        case DWT_DW3000_DEV_ID:
        case DWT_QM33110_DEV_ID:
            return AOA_NOT_SUPPORTED;
        default:
            return AOA_ERROR;
    }
}
