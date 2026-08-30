/**
 * @file      platform_l1_config_helpers.c
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
#include "llhw.h"

#include "OTP_config.h"

#define QLOG_TAG "helper_platform_l1_config"
#include "qlog.h"

/* Character index used for ant_path index or channel index. */
#define ANT_PATH_IDX_CHAR_NUM 3
#define CHAN_IDX_CHAR_NUM     7

void configure_antenna_path(uint8_t ant_path_idx, uint8_t transceiver, uint8_t port)
{
    char key_str_transceiver[] = "ant0.transceiver";
    char key_str_port[] = "ant0.port";
    if (ant_path_idx != 0)
    {
        key_str_transceiver[ANT_PATH_IDX_CHAR_NUM] = '0' + ant_path_idx;
        key_str_port[ANT_PATH_IDX_CHAR_NUM] = '0' + ant_path_idx;
    }
    l1_config_store_key(key_str_transceiver, &transceiver, sizeof(transceiver));
    l1_config_store_key(key_str_port, &port, sizeof(port));
}

void configure_ant_delay(uint8_t ant_path_idx, uint8_t channel, uint32_t ant_delay)
{
    char key_str_ant_delay[] = "ant0.ch5.ant_delay";

    key_str_ant_delay[ANT_PATH_IDX_CHAR_NUM] = '0' + ant_path_idx;
    key_str_ant_delay[CHAN_IDX_CHAR_NUM] = '0' + channel;

    if (CHECK_ANT_DELAY(ant_delay))
        l1_config_store_key(key_str_ant_delay, &ant_delay, sizeof(ant_delay));
}

enum qerr configure_tx_power_index(uint8_t ant_path_idx, uint8_t channel, uint32_t tx_power)
{
    enum qerr r;
    uint32_t tx_power_idx;
    uint8_t tx_power_idx_u8;
    uint8_t tx_power_u8;

    /* TX power index to be configured only for TX antenna, i.e. ant_path_idx 0. */
    char key_str_tx_power_idx[] = "ant0.ch5.ref_frame0.tx_power_index";

    /* Check OTP value validity. */
    if (!CHECK_TX_POWER(tx_power))
        return QERR_SUCCESS;

    /* Conversion from TX power to index requires that same value is calibrated for all TX power
     * sections. */
    tx_power_u8 = tx_power & 0xFF;
    if ((((tx_power >> 8) & 0xFF) != tx_power_u8) || (((tx_power >> 16) & 0xFF) != tx_power_u8) || (((tx_power >> 24) & 0xFF) != tx_power_u8))
        return QERR_EINVAL;

    key_str_tx_power_idx[ANT_PATH_IDX_CHAR_NUM] = '0' + ant_path_idx;
    key_str_tx_power_idx[CHAN_IDX_CHAR_NUM] = '0' + channel;

    r = llhw_convert_tx_power_to_index(channel, tx_power_u8, &tx_power_idx_u8);
    if (r)
        return r;

    tx_power_idx = tx_power_idx_u8 | ((uint32_t)tx_power_idx_u8 << 8) | ((uint32_t)tx_power_idx_u8 << 16) | ((uint32_t)tx_power_idx_u8 << 24);

    l1_config_store_key(key_str_tx_power_idx, &tx_power_idx, sizeof(tx_power_idx));

    return QERR_SUCCESS;
}
