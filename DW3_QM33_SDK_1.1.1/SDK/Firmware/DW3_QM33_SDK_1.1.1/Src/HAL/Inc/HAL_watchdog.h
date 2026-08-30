/**
 * @file      HAL_watchdog.h
 *
 * @brief     Interface for HAL_watchdog
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

/** @brief HAL WATCHDOG API structure */
struct hal_watchdog_s
{
    void (*init)(int ms);
    void (*refresh)(void);
};

extern const struct hal_watchdog_s Watchdog;
