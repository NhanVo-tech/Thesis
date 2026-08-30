/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qerr.h>
#include <stdint.h>

/**
 * qflash_write() - Write an address in flash memory.
 *
 * Allowed values for dst_addr are implementation dependent. Depending on the flash and the
 * implementation, the parameter might have to be aligned to 2 or 4 bytes or a sector or a
 * block.
 *
 * @dst_addr: Destination address. Allowed value is implementation dependent.
 * @src_addr: Source address.
 * @size: Size of the memory to write.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qflash_write(uint32_t dst_addr, void *src_addr, uint32_t size);
