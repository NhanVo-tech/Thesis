/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qerr.h>
#include <stdint.h>

/**
 * qotp_read() - Read the OTP data from given address into provided array
 *
 * @address: OTP address to read from
 * @data:    pointer to the array into which to read the data
 * @length:  number of 32 bit words to read (array needs to be at least this length)
 *
 * Return: QERR_SUCCESS or error
 */
enum qerr qotp_read(uint32_t address, uint32_t *data, uint8_t length);

/**
 * qotp_write() - Write the data to given address in OTP memory.
 *
 * @value:   data to write
 * @address: address to write
 *
 * Return:  QERR_SUCCESS or error
 */
enum qerr qotp_write(uint32_t value, uint32_t address);
