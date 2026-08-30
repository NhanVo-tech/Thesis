/**
 * @file      platform_l1_config.c
 *
 * @brief     L1 config functionalities
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "l1_config.h"
#include "l1_config_types.h"
#include "qotp.h"

#include "helper_platform_l1_config.h"

#define QLOG_TAG "platform_l1_config"
#include "qlog.h"

#define PLATFORM_ID_TYPE2AB 0 /* TODO: Murata Type2AB platform ID. */

/* Valid for all OTP Revs. */
#define OTP_CH5_TXPOWER_ADDRESS     0x11
#define OTP_CH9_TXPOWER_ADDRESS     0x13
#define OTP_PDOA_OFFSET_ADDRESS     0x1A
#define OTP_PGCNT_ADDRESS           0x1B
#define OTP_ANT_DELAY_ADDRESS       0x1C
#define OTP_PGDLY_XTAL_TRIM_ADDRESS 0x1E
#define OTP_REV_ADDRESS             0x1F
#define PLATFORM_ID_ADDRESS         0x5B
#define XTAL_TRIM_MASK              0x7F

/*
 * Three antenna paths:
 *  - Ant path 0: TX RF1
 *  - Ant path 1: RX RF1
 *  - Ant path 2: RX RF2
 */
#define ANT_PATH_NUM 3

/* PDOA LUT for CH5 patch antenna. */
pdoa_lut_entry default_lut_ch5_patch[L1_CONFIG_PDOA_LUT_MAX] = {
    /* clang-format off */
    {0xebd6, 0xf36f},
    {0xebd7, 0xf445},
    {0xebe2, 0xf51c},
    {0xec04, 0xf5f2},
    {0xec44, 0xf6c9},
    {0xec91, 0xf79f},
    {0xed22, 0xf876},
    {0xeded, 0xf94c},
    {0xeedc, 0xfa23},
    {0xeff3, 0xfaf9},
    {0xf175, 0xfbd0},
    {0xf38f, 0xfca6},
    {0xf60a, 0xfd7d},
    {0xf922, 0xfe53},
    {0xfc85, 0xff2a},
    {0x0000, 0x0000},
    {0x02ef, 0x00d6},
    {0x057e, 0x01ad},
    {0x07ca, 0x0283},
    {0x09bd, 0x035a},
    {0x0b50, 0x0430},
    {0x0c43, 0x0507},
    {0x0ceb, 0x05dd},
    {0x0d6e, 0x06b4},
    {0x0e08, 0x078a},
    {0x0eae, 0x0861},
    {0x0f6f, 0x0937},
    {0x1005, 0x0a0e},
    {0x106b, 0x0ae4},
    {0x1090, 0x0bbb},
    {0x1093, 0x0c91}
    /* clang-format on */
};

/* PDOA LUT for CH9 patch antenna. */
pdoa_lut_entry default_lut_ch9_patch[L1_CONFIG_PDOA_LUT_MAX] = {
    /* clang-format off */
    {0xe9d5, 0xf36f},
    {0xea03, 0xf445},
    {0xea59, 0xf51c},
    {0xeb16, 0xf5f2},
    {0xebc8, 0xf6c9},
    {0xecad, 0xf79f},
    {0xed16, 0xf876},
    {0xee47, 0xf94c},
    {0xefd3, 0xfa23},
    {0xf154, 0xfaf9},
    {0xf307, 0xfbd0},
    {0xf56f, 0xfca6},
    {0xf7dc, 0xfd7d},
    {0xfa9e, 0xfe53},
    {0xfd44, 0xff2a},
    {0x0000, 0x0000},
    {0x02ab, 0x00d6},
    {0x0562, 0x01ad},
    {0x07e8, 0x0283},
    {0x0a3d, 0x035a},
    {0x0c5e, 0x0430},
    {0x0e4b, 0x0507},
    {0x0f9f, 0x05dd},
    {0x1109, 0x06b4},
    {0x1286, 0x078a},
    {0x141b, 0x0861},
    {0x15f9, 0x0937},
    {0x17a8, 0x0a0e},
    {0x18e7, 0x0ae4},
    {0x1929, 0x0bbb},
    {0x192a, 0x0c91}
    /* clang-format on */
};

enum qerr l1_config_platform_ops_reset_to_default(void)
{
    enum qerr r;
    uint32_t val32;
    uint8_t val8;
    uint32_t ant_delay_ch5;
    uint32_t ant_delay_ch9;
    uint8_t ant_paths[PAIR_NUM] = {1, 2};
    uint8_t tx_power_control = 0x01;
    int8_t rx_ants[ANTENNA_SET_MAX_NB_RX_ANTS] = {0, ANT_PATH_IDX_INVALID, ANT_PATH_IDX_INVALID};
    bool rx_ants_are_pairs = true;

    /* Set antenna 0 as TX antenna, RF1. */
    configure_antenna_path(0, TRANSCEIVER_TX, 1);

    /* Set antenna 1 as RX antenna, RF1. */
    configure_antenna_path(1, TRANSCEIVER_RXA, 1);

    /* Set antenna 2 as RX antenna, RF2. */
    configure_antenna_path(2, TRANSCEIVER_RXA, 2);

    /* Create an antenna pair. */
    l1_config_store_key("ant_pair0.ant_paths", ant_paths, PAIR_NUM);

    /* Use antenna pair in antenna set 0. */
    l1_config_store_key("ant_set0.rx_ants_are_pairs", &rx_ants_are_pairs, sizeof(rx_ants_are_pairs));
    l1_config_store_key("ant_set0.rx_ants", rx_ants, ANTENNA_SET_MAX_NB_RX_ANTS);

    /* Store default pdoa lut for patch antenna. */
    l1_config_store_key("pdoa_lut0.data", &default_lut_ch5_patch, sizeof(default_lut_ch5_patch));
    l1_config_store_key("pdoa_lut1.data", &default_lut_ch9_patch, sizeof(default_lut_ch9_patch));

    /* Read antenna delay for both channels. */
    r = qotp_read(OTP_ANT_DELAY_ADDRESS, &val32, 1);
    if (r)
        return r;
    /* Same antenna delay is used for TX and RX antennas, but differs from ch5 to ch9 */
    ant_delay_ch5 = (val32 >> 16) & 0xFFFF;
    ant_delay_ch9 = val32 & 0xFFFF;
    for (int i = 0; i < ANT_PATH_NUM; i++)
    {
        configure_ant_delay(i, 5, ant_delay_ch5);
        configure_ant_delay(i, 9, ant_delay_ch9);
    }

    /* Read TX power for channel 5, and convert it to TX power index. */
    r = qotp_read(OTP_CH5_TXPOWER_ADDRESS, &val32, 1);
    if (r)
        return r;
    /* Configure TX power only for Ant path 0, which is TX ant path. */
    r = configure_tx_power_index(0, 5, val32);
    if (r)
        return r;

    /* Read TX power for channel 9, and convert it to TX power index. */
    r = qotp_read(OTP_CH9_TXPOWER_ADDRESS, &val32, 1);
    if (r)
        return r;
    r = configure_tx_power_index(0, 9, val32);
    if (r)
        return r;

    /* PDoA offset only calibrated for OTP Rev 2. */
    int16_t pdoa_offset;

    /* Read PDoA Offset. */
    r = qotp_read(OTP_PDOA_OFFSET_ADDRESS, &val32, 1);
    if (r)
        return r;
    pdoa_offset = (val32 >> 16) & 0xFFFF;
    l1_config_store_key("ant_pair0.ch5.pdoa.offset", &pdoa_offset, sizeof(pdoa_offset));
    pdoa_offset = val32 & 0xFFFF;
    l1_config_store_key("ant_pair0.ch9.pdoa.offset", &pdoa_offset, sizeof(pdoa_offset));

    /* Read xtal trim and pd_delay address. */
    r = qotp_read(OTP_PGDLY_XTAL_TRIM_ADDRESS, &val32, 1);
    if (r)
        return r;
    /* Set xtal_trim. Ignore value if 0. */
    val8 = val32 & XTAL_TRIM_MASK;
    if (val8)
        l1_config_store_key("xtal_trim", &val8, sizeof(val8));

    /* Store pg_delay for both channels for TX antenna path only (ant0). */
    val8 = (val32 >> 24) & 0xFF;
    if (val8)
        l1_config_store_key("ant0.ch5.pg_delay", &val8, sizeof(val8));
    val8 = (val32 >> 16) & 0xFF;
    if (val8)
        l1_config_store_key("ant0.ch9.pg_delay", &val8, sizeof(val8));

    /* Read pg_count for both channels and store it for TX antenna path only (ant0). */
    r = qotp_read(OTP_PGCNT_ADDRESS, &val32, 1);
    if (r)
        return r;
    val8 = (val32 >> 16) & 0xFF;
    if (val8)
        l1_config_store_key("ant0.ch5.pg_count", &val8, sizeof(val8));
    val8 = val32 & 0xFF;
    if (val8)
        l1_config_store_key("ant0.ch9.pg_count", &val8, sizeof(val8));

    /* Activate adaptative TX power. */
    l1_config_store_key("ant_set0.tx_power_control", &tx_power_control, sizeof(tx_power_control));

    return QERR_SUCCESS;
}

struct l1_config_platform_ops l1_config_platform_ops = {
    .reset_to_default = l1_config_platform_ops_reset_to_default,
};
