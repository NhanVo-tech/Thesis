/**
 * @file      helper_platform_l1_config.h
 *
 * @brief     Helpers to develop platform specific L1 Config ops
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "l1_config.h"
#include "l1_config_types.h"

/**
 * configure_antenna_path() - Configure an antenna path.
 * @ant_path_idx: Index of the antenna path to configure.
 * @transceiver: Transceiver to use for the antenna path.
 * @port: RF port to use for the antenna path.
 */
void configure_antenna_path(uint8_t ant_path_idx, uint8_t transceiver, uint8_t port);

/**
 * configure_ant_delay() - Configure an antenna delay for a specific antenna path and channel.
 * @ant_path_idx: Index of the antenna path for which to configure the antenna delay.
 * @channel: Channel for which to configure the antenna delay.
 * @ant_delay: Antenna delay to configure.
 */
void configure_ant_delay(uint8_t ant_path_idx, uint8_t channel, uint32_t ant_delay);

/**
 * configure_tx_power_index() - Configure TX power index for a specific antenna path and channel.
 * @ant_path_idx: Index of the antenna path for which to configure the TX power.
 * @channel: Channel for which to configure the TX power.
 * @tx_power: TX power to configure.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr configure_tx_power_index(uint8_t ant_path_idx, uint8_t channel, uint32_t tx_power);
