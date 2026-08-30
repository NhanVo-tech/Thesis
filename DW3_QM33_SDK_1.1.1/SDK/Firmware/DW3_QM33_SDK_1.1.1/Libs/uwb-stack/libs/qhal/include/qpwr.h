/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qerr.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * qpwr_enable_lpm() - Enable low power mode.
 */
void qpwr_enable_lpm(void);

/**
 * qpwr_disable_lpm() - Disable low power mode.
 */
void qpwr_disable_lpm(void);

/**
 * qpwr_is_lpm_enabled() - Check if low power mode is enabled.
 *
 * Return: True if low power mode is enabled, otherwise false.
 */
bool qpwr_is_lpm_enabled(void);

/**
 * qpwr_set_min_inactivity_s4() - Set the minimum inactivity time to get in S4.
 *
 * @time_ms: minimum inactivity time to get in S4, in ms.
 *
 * Return: QERR_SUCCESS or negative QERR error
 */
enum qerr qpwr_set_min_inactivity_s4(uint32_t time_ms);

/**
 * qpwr_get_min_inactivity_s4() - Get the minimum inactivity time to get in S4.
 *
 * @time_ms: minimum inactivity time to get in S4, in ms.
 *
 * Return: QERR_SUCCESS or negative QERR error
 */
enum qerr qpwr_get_min_inactivity_s4(uint32_t *time_ms);

/**
 * qpwr_uwb_sleep() - Put UWB to sleep. Do nothing if it is already sleeping.
 *
 * Return: QERR_SUCCESS or negative QERR error
 */
enum qerr qpwr_uwb_sleep(void);

/**
 * qpwr_uwb_wakeup() - Wake up UWB from sleep. Do nothing if it is already awake.
 *
 * Return: QERR_SUCCESS or negative QERR error
 */
enum qerr qpwr_uwb_wakeup(void);

/**
 * qpwr_uwb_is_sleeping() - Check if UWB is sleeping.
 *
 * Return: true if UWB is sleeping, otherwise false.
 */
bool qpwr_uwb_is_sleeping(void);
