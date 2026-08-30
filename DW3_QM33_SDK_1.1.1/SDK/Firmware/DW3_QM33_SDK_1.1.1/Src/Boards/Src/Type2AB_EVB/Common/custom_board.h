/**
 * @file      custom_board.h
 *
 * @brief     Pin mapping description for Type2AB EVB
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

#include "type2ab_evb.h"
#include "nrf_spim.h"

#define LED_ERROR           BSP_LED_0

#define DW3000_MIN_SPI_FREQ (NRF_SPIM_FREQ_4M)
#define DW3000_MAX_SPI_FREQ (NRF_SPIM_FREQ_32M)
#define DW3000_SPI_IDX      (3)

#ifdef __cplusplus
}
#endif
