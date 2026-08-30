/**
 * @file      platform_l1_config.c
 *
 * @brief     L1 config functionalities
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "l1_config.h"
#include "l1_config_types.h"
#include "qotp.h"

#include "helper_platform_l1_config.h"

#define QLOG_TAG "platform_l1_config"
#include "qlog.h"

#define PLATFORM_ID_DW3000   0
#define PLATFORM_ID_DW3001C  1

#define CAL_REV_DW3000       0
#define CAL_REV_DW3001C_PVT2 1
#define CAL_REV_DW3001C_MP   2

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

static void configure_ant_delays_from_otp(uint8_t channel, uint32_t ant_delay_otp)
{
    uint32_t tx_ant_delay;
    uint32_t rx_ant_delay;

    tx_ant_delay = ant_delay_otp & 0xFFFF;
    rx_ant_delay = (ant_delay_otp >> 16) & 0xFFFF;

    /* For TX antenna delay: use ant_path_idx 0.
     * For RX antenna delay: use ant_path_idx 1. */
    configure_ant_delay(0, channel, tx_ant_delay);
    configure_ant_delay(1, channel, rx_ant_delay);
}

enum qerr l1_config_platform_ops_reset_to_default(void)
{
    enum qerr r;
    uint32_t val32;
    uint16_t platform_id;
    uint8_t xtal_trim;
    uint8_t cal_rev;
    uint8_t tx_power_control = 0x01;

    /* Retrieve OTP revision. */
    r = qotp_read(OTP_REV_ADDRESS, &val32, 1);
    if (r)
        return r;
    platform_id = val32 >> 16;
    cal_rev = val32 >> 8;

    /* Sanity check. */
    if ((platform_id != PLATFORM_ID_DW3000) && (platform_id != PLATFORM_ID_DW3001C))
        return QERR_EIO;

    /* Set antenna 0 as TX antenna, RF1. */
    configure_antenna_path(0, TRANSCEIVER_TX, 1);

    /* Set antenna 1 as RX antenna, RF1. */
    configure_antenna_path(1, TRANSCEIVER_RXA, 1);

    /* Read and set antenna delay for channel 5. */
    if ((platform_id == PLATFORM_ID_DW3001C) && (cal_rev == CAL_REV_DW3001C_PVT2))
    {
        /* Specific workaround: on channel 5, for DW3001C and cal rev 1, do not use the OTP value
         * but a default common value which has been defined after characterization. */
        val32 = CH5_ANTDELAY_DW3001C_PVT2;
    }
    else
    {
        r = qotp_read(OTP_CH5_ANTDELAY_ADDRESS, &val32, 1);
        if (r)
            return r;
    }
    configure_ant_delays_from_otp(5, val32);

    /* Read and set antenna delay for channel 9. */
    r = qotp_read(OTP_CH9_ANTDELAY_ADDRESS, &val32, 1);
    if (r)
        return r;
    configure_ant_delays_from_otp(9, val32);

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
