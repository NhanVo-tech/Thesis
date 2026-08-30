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

#define PLATFORM_ID_QM33110 3 /* QM33110 (no PDOA) platform ID. */
#define PLATFORM_ID_QM33120 2 /* QM33120 (with PDOA) platform ID. */
#define PLATFORM_ID_DW3000  0 /* DW3000 platform ID. */
#define PLATFORM_ID_DW3001C 1 /* DW3001C platform ID. */

#define CAL_REV_DW3001C_PVT2 1

/* On DW3001C_PVT2, set a common value for Antenna delay, on channel 5 only.
 * Use value 16366 for both TX and RX. */
#define CH5_ANTDELAY_DW3001C_PVT2 0x3FEE3FEE

/* Valid for all OTP Revs. */
#define OTP_CH5_TXPOWER_ADDRESS  0x11
#define OTP_CH9_TXPOWER_ADDRESS  0x13
#define OTP_PGCNT_ADDRESS        0x18
#define OTP_CH5_ANTDELAY_ADDRESS 0x1A
#define OTP_CH9_ANTDELAY_ADDRESS 0x1C
#define OTP_XTAL_TRIM_ADDRESS    0x1E
#define OTP_REV_ADDRESS          0x1F
#define XTAL_TRIM_MASK           0x7F

/* Valid only for OTP Rev 2. */
#define OTP_REV_PDOA_SUPPORT 2
#define OTP_PDOA_ADDRESS     0x14

/* PDOA LUT for CH5 jolie antenna. */
pdoa_lut_entry default_lut_ch5_jolie[L1_CONFIG_PDOA_LUT_MAX] = {
    /* clang-format off */
    {0xe8c8, 0xf36f},
    {0xea49, 0xf445},
    {0xeb88, 0xf51c},
    {0xeccc, 0xf5f2},
    {0xedf8, 0xf6c9},
    {0xef1d, 0xf79f},
    {0xf080, 0xf876},
    {0xf1c6, 0xf94c},
    {0xf33d, 0xfa23},
    {0xf4de, 0xfaf9},
    {0xf6b5, 0xfbd0},
    {0xf8a3, 0xfca6},
    {0xfa96, 0xfd7d},
    {0xfc83, 0xfe53},
    {0xfe4c, 0xff2a},
    {0x0000, 0x0000},
    {0x017f, 0x00d6},
    {0x033f, 0x01ad},
    {0x050b, 0x0283},
    {0x0701, 0x035a},
    {0x090c, 0x0430},
    {0x0b55, 0x0507},
    {0x0da0, 0x05dd},
    {0x0fda, 0x06b4},
    {0x11e2, 0x078a},
    {0x13b2, 0x0861},
    {0x153c, 0x0937},
    {0x166f, 0x0a0e},
    {0x177d, 0x0ae4},
    {0x1864, 0x0bbb},
    {0x18ea, 0x0c91}
    /* clang-format on */
};

/* PDOA LUT for CH9 jolie antenna. */
pdoa_lut_entry default_lut_ch9_jolie[L1_CONFIG_PDOA_LUT_MAX] = {
    /* clang-format off */
    {0xe4e0, 0xf36f},
    {0xe59a, 0xf445},
    {0xe66b, 0xf51c},
    {0xe75e, 0xf5f2},
    {0xe88a, 0xf6c9},
    {0xea0e, 0xf79f},
    {0xebe4, 0xf876},
    {0xedf5, 0xf94c},
    {0xf02b, 0xfa23},
    {0xf26e, 0xfaf9},
    {0xf4cc, 0xfbd0},
    {0xf709, 0xfca6},
    {0xf946, 0xfd7d},
    {0xfb7f, 0xfe53},
    {0xfdce, 0xff2a},
    {0x0000, 0x0000},
    {0x0237, 0x00d6},
    {0x0471, 0x01ad},
    {0x06ac, 0x0283},
    {0x08d7, 0x035a},
    {0x0acc, 0x0430},
    {0x0cbb, 0x0507},
    {0x0e6e, 0x05dd},
    {0x1008, 0x06b4},
    {0x1194, 0x078a},
    {0x1306, 0x0861},
    {0x1427, 0x0937},
    {0x1534, 0x0a0e},
    {0x161f, 0x0ae4},
    {0x16c3, 0x0bbb},
    {0x1725, 0x0c91}
    /* clang-format on */
};

static void configure_ant_delays_from_otp(uint8_t channel, uint32_t ant_delay_otp, bool support_aoa)
{
    uint32_t tx_ant_delay;
    uint32_t rx_ant_delay;

    tx_ant_delay = ant_delay_otp & 0xFFFF;
    rx_ant_delay = (ant_delay_otp >> 16) & 0xFFFF;

    /* For TX antenna delay: use ant_path_idx 0.
     * For RX antenna delay: use ant_path_idx 1, and 2 if AoA. */
    configure_ant_delay(0, channel, tx_ant_delay);
    configure_ant_delay(1, channel, rx_ant_delay);

    if (support_aoa)
        configure_ant_delay(2, channel, rx_ant_delay);
}

enum qerr l1_config_platform_ops_reset_to_default(void)
{
    enum qerr r;
    uint32_t val32;
    uint16_t platform_id;
    uint8_t otp_rev;
    uint8_t cal_rev;
    uint8_t xtal_trim;
    bool support_aoa;
    uint8_t tx_power_control = 0x01;

    /* Retrieve OTP revision. */
    r = qotp_read(OTP_REV_ADDRESS, &val32, 1);
    if (r)
        return r;
    otp_rev = val32 & 0xFF;
    cal_rev = (val32 >> 8) & 0xFF;
    platform_id = val32 >> 16;

    if (platform_id == PLATFORM_ID_QM33120)
        support_aoa = true;
    else if (platform_id == PLATFORM_ID_QM33110)
        support_aoa = false;
    else if ((platform_id == PLATFORM_ID_DW3000) || (platform_id == PLATFORM_ID_DW3001C))
        support_aoa = false;
    else
        return QERR_SUCCESS;

    /* Set antenna 0 as TX antenna, RF1. */
    configure_antenna_path(0, TRANSCEIVER_TX, 1);

    /* Set antenna 1 as RX antenna, RF1. */
    configure_antenna_path(1, TRANSCEIVER_RXA, 1);

    if (support_aoa)
    {
        /* Set antenna 2 as RX antenna, RF2, only for PDoA version. */
        configure_antenna_path(2, TRANSCEIVER_RXA, 2);
    }

    /* Read and set antenna delay for channel 5. */
    if ((platform_id == PLATFORM_ID_DW3001C) && (cal_rev == CAL_REV_DW3001C_PVT2))
    {
        val32 = CH5_ANTDELAY_DW3001C_PVT2;
    }
    else
    {
        r = qotp_read(OTP_CH5_ANTDELAY_ADDRESS, &val32, 1);
        if (r)
            return r;
    }
    configure_ant_delays_from_otp(5, val32, support_aoa);

    /* Read and set antenna delay for channel 9. */
    r = qotp_read(OTP_CH9_ANTDELAY_ADDRESS, &val32, 1);
    if (r)
        return r;
    configure_ant_delays_from_otp(9, val32, support_aoa);

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

    if (support_aoa)
    {
        uint8_t ant_paths[PAIR_NUM] = {1, 2};
        int8_t rx_ants[ANTENNA_SET_MAX_NB_RX_ANTS] = {0, ANT_PATH_IDX_INVALID, ANT_PATH_IDX_INVALID};
        bool rx_ants_are_pairs = true;

        /* Create an antenna pair. */
        l1_config_store_key("ant_pair0.ant_paths", ant_paths, PAIR_NUM);

        /* Use antenna pair in antenna set 0. */
        l1_config_store_key("ant_set0.rx_ants_are_pairs", &rx_ants_are_pairs, sizeof(rx_ants_are_pairs));
        l1_config_store_key("ant_set0.rx_ants", rx_ants, ANTENNA_SET_MAX_NB_RX_ANTS);

        /* Store default pdoa lut for jolie antenna. */
        l1_config_store_key("pdoa_lut0.data", &default_lut_ch5_jolie, sizeof(default_lut_ch5_jolie));
        l1_config_store_key("pdoa_lut1.data", &default_lut_ch9_jolie, sizeof(default_lut_ch9_jolie));

        /* PDoA offset only calibrated for OTP Rev 2. */
        if (otp_rev == OTP_REV_PDOA_SUPPORT)
        {
            int16_t pdoa_offset;

            /* Read PDoA Offset. */
            r = qotp_read(OTP_PDOA_ADDRESS, &val32, 1);
            if (r)
                return r;

            pdoa_offset = (val32 >> 16) & 0xFFFF;
            l1_config_store_key("ant_pair0.ch5.pdoa.offset", &pdoa_offset, sizeof(pdoa_offset));
            pdoa_offset = val32 & 0xFFFF;
            l1_config_store_key("ant_pair0.ch9.pdoa.offset", &pdoa_offset, sizeof(pdoa_offset));
        }
    }

    /* Read xtal trim. Ignore value if 0. */
    r = qotp_read(OTP_XTAL_TRIM_ADDRESS, &val32, 1);
    if (r)
        return r;
    xtal_trim = val32 & XTAL_TRIM_MASK;
    if (xtal_trim != 0)
        l1_config_store_key("xtal_trim", &xtal_trim, sizeof(xtal_trim));

    /* Read pg_count for both channels and store it for TX antenna path only (ant0). */
    r = qotp_read(OTP_PGCNT_ADDRESS, &val32, 1);
    if (r)
        return r;
    if (val32 != 0)
    {
        uint8_t pg_count;
        pg_count = (val32 >> 16) & 0xFF;
        l1_config_store_key("ant0.ch5.pg_count", &pg_count, sizeof(pg_count));
        pg_count = val32 & 0xFF;
        l1_config_store_key("ant0.ch9.pg_count", &pg_count, sizeof(pg_count));
    }

    /* Activate adaptative TX power. */
    l1_config_store_key("ant_set0.tx_power_control", &tx_power_control, sizeof(tx_power_control));

    return QERR_SUCCESS;
}

struct l1_config_platform_ops l1_config_platform_ops = {
    .reset_to_default = l1_config_platform_ops_reset_to_default,
};
