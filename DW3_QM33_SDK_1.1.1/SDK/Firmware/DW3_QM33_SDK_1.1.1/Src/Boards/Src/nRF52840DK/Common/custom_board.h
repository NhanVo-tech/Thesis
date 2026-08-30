/**
 * @file      custom_board.h
 *
 * @brief     Pin mapping description corresponding to nRF52840DK module
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

#include "pca10056.h"
#include "nrf_spim.h"


/* UART symbolic constants. */
#define UART_0_TX_PIN TX_PIN_NUMBER
#define UART_0_RX_PIN RX_PIN_NUMBER

#define LED_ERROR     BSP_LED_0

#ifdef __cplusplus
}
#endif
