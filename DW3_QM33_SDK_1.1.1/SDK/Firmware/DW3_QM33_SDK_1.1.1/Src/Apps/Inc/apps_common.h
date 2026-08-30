/**
 * @file      apps_common.h
 *
 * @brief     Only common macros and enumerations
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

#define TS_40B_SIZE         (5)

#define PHY_READ_DIG_ENABLE (0x1)
#define PHY_READ_ACC_ENABLE (0x2)
/* In m/s in air. */
#define SPEED_OF_LIGHT (299702547.0)

/* Common macros. */
#ifndef SWAP
#define SWAP(a, b) \
    {              \
        a ^= b;    \
        b ^= a;    \
        a ^= b;    \
    }
#endif /* SWAP */

#ifndef TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef FALSE
#define FALSE 0
#endif /* FALSE */

/* Counter is 40 bits. */
#define MASK_40BIT (0x00FFFFFFFFFFULL)
/* The TX timestamp will snap to 8 ns resolution - mask lower 9 bits. */
#define MASK_TXDTS (0x00FFFFFFFE00ULL)


#ifdef __cplusplus
}
#endif
