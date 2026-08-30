/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "l1_config.h"
#include "llhw_config.h"
#include "llhw_drv_config.h"
#include "net/mcps802154.h"

#include <qatomic.h>
#include <qmutex.h>
#include <qtoolchain.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * llhw_init() - Init Low-Level Hardware layer.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr llhw_init(void);

/**
 * llhw_deinit() - DeInit Low-Level Hardware layer.
 *
 */
void llhw_deinit(void);

/**
 * llhw_lpm_enter() - Prepare llhw to enter in Low Power Mode.
 *
 */
void llhw_lpm_enter(void);

/**
 * llhw_lpm_exit() - DeInit Low-Level Hardware layer.
 *
 */
void llhw_lpm_exit(void);

/**
 * llhw_convert_tx_power_to_index() - Convert a TX power value into its corresponding power index.
 * @channel: the channel for which the TX power index must be calculated.
 * @tx_power: the TX power to convert.
 * @tx_power_idx: the returned TX power index.
 *
 * The TX power index is a linear value where one unit means an attenuation of 0.25 dB compared to
 * the maximum emitted power. The correspondance between an index and its corresponding TX power
 * has been characterized in order to reduce the LO leakeage and maximize effective dynamics.
 * The L1 configuration uses TX power indexes.
 * For compatibility with chips which were calibrated using old model (TX power instead of index),
 * the conversion from TX power to index is required.
 *
 * Note: it is considered that the same TX power value is used for all parts of the frame.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr llhw_convert_tx_power_to_index(int channel, uint8_t tx_power, uint8_t *tx_power_idx);

#ifdef __cplusplus
}
#endif
