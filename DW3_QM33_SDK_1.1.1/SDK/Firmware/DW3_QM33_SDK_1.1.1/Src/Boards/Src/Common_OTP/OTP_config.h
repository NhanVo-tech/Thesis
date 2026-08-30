/*
 * @file      OTP_config.h
 *
 * @brief     Common functions to check and use values from OTP
 *            Functions will be use in restore_bssConfig()
 *            NOTE: The code is very MCU dependent and used only with the QM33 for now
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

/* From OTP Rev 1 and 2 */
#define OTP_CH5_TXPOWER_ADDRESS  0x11
#define OTP_CH9_TXPOWER_ADDRESS  0x13
#define OTP_CH5_ANTDELAY_ADDRESS 0x1A
#define OTP_CH9_ANTDELAY_ADDRESS 0x1C
#define OTPREV_ADDRESS           0x1F

/* From OTP Rev 2 */
#define OTP_PDOA_ADDRESS 0x14

/* Validity check for Antenna delay. */
#define ANTENNA_DELAY_MAX_ALLOWED_VALUE 0x4268
#define ANTENNA_DELAY_MIN_ALLOWED_VALUE 0x3E80

#define CHECK_ANT_DELAY(x)              ((x >= ANTENNA_DELAY_MIN_ALLOWED_VALUE) && (x <= ANTENNA_DELAY_MAX_ALLOWED_VALUE))

/* Validity check for TX Power. */
#define TX_POWER_MAX_ALLOWED_VALUE 0xFEFEFEFE
#define TX_POWER_MIN_ALLOWED_VALUE 0x01010101

#define CHECK_TX_POWER(x)          ((x >= TX_POWER_MIN_ALLOWED_VALUE) && (x <= TX_POWER_MAX_ALLOWED_VALUE))
