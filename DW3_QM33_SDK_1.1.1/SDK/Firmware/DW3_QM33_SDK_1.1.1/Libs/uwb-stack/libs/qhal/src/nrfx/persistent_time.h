/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <stdint.h>

/** base_time_rollover_handler() - Base time rollover handler.
 *
 *  This function is called when the base time has rolled over.
 *
 *  Return: Nothing.
 */
void base_time_rollover_handler(void *);

/* TODO: Needed as first version of persistent_time was done for Zephyr
 * Needs to be changed when final API is decided.
 */
enum pm_state {
	AVOID_EMPTY_ENUM,
};

/**
 * persistent_time_init() - Initalize persistent time base.
 * @last_state: Device last power state.
 *
 * Return: Nothing.
 */
void persistent_time_init(enum pm_state last_state);

/**
 * persistent_time_get_rtc_us() - Get RTC value in microseconds.
 *
 * Return: 64-bits RTC value (in microseconds).
 */
int64_t persistent_time_get_rtc_us(void);

/**
 * persistent_time_resync_rtc_systime() - Resync system time with RTC time.
 * @rtc_us: Output value with current rtc time.
 * @systime: Output value with current systime.
 *
 * Return: Nothing.
 */
void persistent_time_resync_rtc_systime(int64_t *rtc_us, uint32_t *systime);

/**
 * persistent_time_update_rtc_systime() - Update sys and RTC time.
 * @updated_rtc_us: Updated rtc time.
 * @updated_systime: Updated systime.
 *
 * Return: Nothing.
 */
void persistent_time_update_rtc_systime(int64_t updated_rtc_us, uint32_t updated_systime);
