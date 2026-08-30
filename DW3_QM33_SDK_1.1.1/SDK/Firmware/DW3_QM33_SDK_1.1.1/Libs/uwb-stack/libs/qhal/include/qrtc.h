/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <stdint.h>

/**
 * qrtc_get_us() - Get RTC value in microseconds.
 *
 * Return: 64-bits RTC value (in microseconds).
 */
int64_t qrtc_get_us(void);

/**
 * qrtc_resync_rtc_systime() - Resync RTC and SysTime.
 * @rtc_us: Returned current RTC value (in microseconds).
 * @systime: Returned current UWB transceiver SysTime (in ticks).
 */
void qrtc_resync_rtc_systime(int64_t *rtc_us, uint32_t *systime);

/**
 * qrtc_update_rtc_systime() - Overwrite current RTC and SysTime used in QRTC.
 * @updated_rtc_us: New current RTC value (in microseconds).
 * @updated_systime: New UWB transceiver SysTime (in ticks).
 *
 * NOTE: that function is used as a workaround when RTC is based on Systime.
 */
void qrtc_update_rtc_systime(int64_t updated_rtc_us, uint32_t updated_systime);
