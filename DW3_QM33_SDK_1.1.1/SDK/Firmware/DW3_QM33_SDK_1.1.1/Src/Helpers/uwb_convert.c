/**
 * @file      uwb_convert.c
 *
 * @brief     Set of function for converting data.
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "uwb_convert.h"

float convert_aoa_2pi_q16_to_deg(int16_t aoa_2pi_q16)
{
    return (360.0 * aoa_2pi_q16 / (1 << 16));
}

float convert_rssi_q7_to_dbm(uint8_t rssi_q7)
{
    return (-1.0 * rssi_q7 / (1 << 1));
}
