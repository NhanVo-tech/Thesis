/**
 * @file      usb_uart_tx.h
 *
 * @brief     Interface for usb_uart_tx
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
#include <stdint.h>
#include "deca_error.h"

error_e copy_tx_msg(uint8_t *str, int len);
error_e flush_report_buf(void);
error_e port_tx_msg(uint8_t *str, int len);
int reset_report_buf(void);
error_e init_tx_msg(void);


#ifdef __cplusplus
}
#endif
