/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "deca_device_api.h"

#include <assert.h>
#include <nrfx_rtc.h>
#include <persistent_time.h>
#include <qassert.h>
#include <qtimer.h>
#include <stdint.h>
#define LOG_TAG "persistent_time_nrfx"
#include "qrtc_share.h"

#include <qlog.h>

#define PERSISTENT_FREQUENCY 32768 /* 32768Hz freq. T = 30.5us. */
#define US_IN_S 1000000UL
#define TICKS_TO_US(ticks) (((uint64_t)(ticks)*US_IN_S) / PERSISTENT_FREQUENCY)
#define RTC_PRESCALER_TO_FREQ(PRESCALER) (uint16_t)((RTC_INPUT_FREQ) / ((PRESCALER) + 1))

/* Macro to check if the PERSISTENT_FREQUENCY value is valid */
#define IS_VALID_FREQUENCY(FREQ) (RTC_PRESCALER_TO_FREQ(RTC_FREQ_TO_PRESCALER(FREQ)) == (FREQ))

/* Compile-time check for PERSISTENT_FREQUENCY */
_Static_assert(
	IS_VALID_FREQUENCY(PERSISTENT_FREQUENCY),
	"Invalid PERSISTENT_FREQUENCY value. Ensure PERSISTENT_FREQUENCY matches the formula: fRTC [Hz] = 32768 / (PRESCALER + 1), where PRESCALER is a 12-bit value.");

static nrfx_rtc_t base_timer = NRFX_RTC_INSTANCE(BASE_TIMER_RTC_INSTANCE);
static uint64_t base_time_rollover_ticks = 0;

static const nrfx_rtc_config_t base_timer_config = { .prescaler = RTC_FREQ_TO_PRESCALER(
							     PERSISTENT_FREQUENCY),
						     .interrupt_priority = 7,
						     .reliable = false,
						     .tick_latency = 0 };

void rtc_overflow_handler(void)
{
	base_time_rollover_ticks += RTC_MAX_COUNT_24BITS;
}

void persistent_time_init(enum pm_state last_state)
{
	(void)last_state;

	if (nrfx_rtc_init(&base_timer, &base_timer_config,
			  (nrfx_rtc_handler_t)BASE_TIMER_INT_HANDLER) != NRFX_SUCCESS) {
		QLOGE("persistent_time: initialization failed for id %d.", BASE_TIMER_RTC_INSTANCE);
		return;
	}

	nrfx_rtc_enable(&base_timer);
	nrfx_rtc_overflow_enable(&base_timer, true);
	QLOGD("persistent_time_init");
}

static inline int64_t wait_synced_ticks(void)
{
	uint32_t ticks, ticks_prev;

	ticks_prev = nrfx_rtc_counter_get(&base_timer);

	/* Wait for tick change to ensure edge alignment */
	do {
		ticks = nrfx_rtc_counter_get(&base_timer);
	} while (ticks == ticks_prev);

	return (int64_t)(ticks + base_time_rollover_ticks);
}

int64_t persistent_time_get_rtc_us(void)
{
	uint32_t ticks = nrfx_rtc_counter_get(&base_timer);
	return TICKS_TO_US((int64_t)(ticks + base_time_rollover_ticks));
}

void persistent_time_resync_rtc_systime(int64_t *rtc_us, uint32_t *systime)
{
	uint32_t cur_systime;
	int64_t ticks;

	ticks = wait_synced_ticks();
	*rtc_us = TICKS_TO_US(ticks);
	cur_systime = dwt_readsystimestamphi32();
	*systime = cur_systime;
}

void persistent_time_update_rtc_systime(int64_t updated_rtc_us, uint32_t updated_systime)
{
}
