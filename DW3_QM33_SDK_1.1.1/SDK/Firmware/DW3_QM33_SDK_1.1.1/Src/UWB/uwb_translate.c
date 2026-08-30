/**
 * @file      uwb_translate.c
 *
 * @brief     Translate UWB device parameters to human format
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "uwb_translate.h"

#include "deca_device_api.h"

int chan_to_deca(int i)
{
    int ret = -1;

    if (i == 5 || i == 9)
    {
        ret = i;
    }

    return (ret);
}

int deca_to_chan(int i)
{
    return (chan_to_deca(i));
}

int bitrate_to_deca(int i)
{
    switch (i)
    {
        case 850:
            return DWT_BR_850K;
        case 6810:
            return DWT_BR_6M8;
        default:
            return -1;
    }
}

int deca_to_bitrate(int i)
{
    switch (i)
    {
        case DWT_BR_850K:
            return 850;
        case DWT_BR_6M8:
            return 6810;
        default:
            return -1;
    }
}

int pac_to_deca(int i)
{
    switch (i)
    {
        case 8:
            return DWT_PAC8;
        case 16:
            return DWT_PAC16;
        case 32:
            return DWT_PAC32;
        case 4:
            return DWT_PAC4;
        default:
            return -1;
    }
}

int deca_to_pac(int i)
{
    switch (i)
    {
        case DWT_PAC8:
            return 8;
        case DWT_PAC16:
            return 16;
        case DWT_PAC32:
            return 32;
        case DWT_PAC4:
            return 4;
        default:
            return -1;
    }
}

int plen_to_deca(int i)
{
    switch (i)
    {
        case 256:
            return DWT_PLEN_256;
        case 128:
            return DWT_PLEN_128;
        case 72:
            return DWT_PLEN_72;
        case 64:
            return DWT_PLEN_64;
        case 32:
            return DWT_PLEN_32;
        default:
            return -1;
    }
}

int deca_to_plen(int i)
{
    switch (i)
    {
        case DWT_PLEN_256:
            return 256;
        case DWT_PLEN_128:
            return 128;
        case DWT_PLEN_72:
            return 72;
        case DWT_PLEN_64:
            return 64;
        case DWT_PLEN_32:
            return 32;
        default:
            return -1;
    }
}

int sts_length_to_deca(int i)
{
    switch (i)
    {
        case 2048:
            return DWT_STS_LEN_2048;
        case 1024:
            return DWT_STS_LEN_1024;
        case 512:
            return DWT_STS_LEN_512;
        case 256:
            return DWT_STS_LEN_256;
        case 128:
            return DWT_STS_LEN_128;
        case 64:
            return DWT_STS_LEN_64;
        case 32:
            return DWT_STS_LEN_32;
        default:
            return -1;
    }
}

int deca_to_sts_length(int i)
{
    switch (i)
    {
        case DWT_STS_LEN_2048:
            return 2048;
        case DWT_STS_LEN_1024:
            return 1024;
        case DWT_STS_LEN_512:
            return 512;
        case DWT_STS_LEN_256:
            return 256;
        case DWT_STS_LEN_128:
            return 128;
        case DWT_STS_LEN_64:
            return 64;
        case DWT_STS_LEN_32:
            return 32;
        default:
            return -1;
    }
}

int preamble_code_to_deca(int i)
{
    switch (i)
    {
        case 9:
        case 10:
        case 11:
        case 12:
            return i;
        default:
            return -1;
    }
}

int deca_to_preamble_code(int i)
{
    return (preamble_code_to_deca(i));
}

int sts_mode_to_deca(int i)
{
    switch (i)
    {
        case DWT_STS_MODE_OFF:
        case DWT_STS_MODE_1:
        case DWT_STS_MODE_2:
        case DWT_STS_MODE_ND:
        case DWT_STS_MODE_SDC:
            return i;
        default:
            return -1;
    }
}

int deca_to_sts_mode(int i)
{
    return (sts_mode_to_deca(i));
}

int sfd_type_to_deca(int i)
{
    switch (i)
    {
        case 0:
        case 1:
        case 2:
        case 3:
            return i;
        default:
            return -1;
    }
}

int deca_to_sfd_type(int i)
{
    return (sfd_type_to_deca(i));
}

int phr_mode_to_deca(int i)
{
    switch (i)
    {
        case DWT_PHRMODE_STD:
        case DWT_PHRMODE_EXT:
            return i;
        default:
            return -1;
    }
}

int deca_to_phr_mode(int i)
{
    return (phr_mode_to_deca(i));
}

int phr_rate_to_deca(int i)
{
    switch (i)
    {
        case DWT_PHRRATE_STD:
        case DWT_PHRRATE_DTA:
            return i;
        default:
            return -1;
    }
}

int deca_to_phr_rate(int i)
{
    return (phr_mode_to_deca(i));
}

int pdoa_mode_to_deca(int i)
{
    switch (i)
    {
        case DWT_PDOA_M0:
        case DWT_PDOA_M1:
        case DWT_PDOA_M3:
            return i;
        default:
            return -1;
    }
}

int deca_to_pdoa_mode(int i)
{
    return (pdoa_mode_to_deca(i));
}
