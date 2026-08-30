/**
 * @file      HAL_lock.h
 *
 * @brief     Interface for HAL_lock
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

/** @brief LOCK state. */
typedef enum
{
    DW_HAL_NODE_UNLOCKED = 0,
    DW_HAL_NODE_LOCKED = 1
} dw_hal_lockTypeDef;


/** @brief Macro to lock handle. */
#define QHAL_LOCK(__HANDLE__)                         \
    do                                                \
    {                                                 \
        if ((__HANDLE__)->lock == DW_HAL_NODE_LOCKED) \
        {                                             \
            return -1;                                \
        }                                             \
        else                                          \
        {                                             \
            (__HANDLE__)->lock = DW_HAL_NODE_LOCKED;  \
        }                                             \
    } while (0U)

/** @brief Macro to unlock handle. */
#define QHAL_UNLOCK(__HANDLE__)                    \
    do                                             \
    {                                              \
        (__HANDLE__)->lock = DW_HAL_NODE_UNLOCKED; \
    } while (0U)
