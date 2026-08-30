/**
 * @file      usb_uart_rx.h
 *
 * @brief     Interface for usb_uart_rx
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include "circular_buffer.h"
#include "app.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_CMD_LENGTH 0x200

usb_data_e usb_uart_rx(struct cc_buff *buf);

#ifdef __cplusplus
}
#endif
