/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#define LOG_TAG "qtimer_nrfx"
#include "persistent_time.h"
#include "qrtc_share.h"

#include <qlog.h>

#if !defined(NRFX_RTC_ENABLED) || (NRFX_RTC_ENABLED == 0)
#error "NRFX RTC module should be enabled."
#endif

extern nrfx_rtc_t rtc_timer;

struct qtimer *qtimer_init(uint8_t qtimer_id, const struct qtimer_config *config, qtimer_cb handler,
			   void *arg)
{
	static uint8_t rtc_cc_number = NRFX_RTC_INT_COMPARE0;

	if (!config) {
		QLOGE("Config is NULL.");
		return NULL;
	}

	if (config->freq_hz > RTC_INPUT_FREQ) {
		QLOGE("RTC frequency cannot be higher than %d.", RTC_INPUT_FREQ);
		return NULL;
	}

	if (config->width != QTIMER_WIDTH_24_BIT) {
		QLOGE("Only 24-bit width is supported for RTC.");
		return NULL;
	}

	struct qtimer_instance *timer_instance = qrtc_find_this_timer(qtimer_id);
	if (!timer_instance) {
		timer_instance = qrtc_find_available_timer();
		if (!timer_instance) {
			QLOGE("No more available instances. Check NRFX_RTC_ENABLED_COUNT.");
			return NULL;
		}

		timer_instance->timer.rtc_cc_instance = rtc_cc_number++;
	}

	if (!RTC_INSTANCE_SHARED) {
		nrfx_rtc_config_t p_config = { .prescaler = RTC_FREQ_TO_PRESCALER(RTC_INPUT_FREQ),
					       .interrupt_priority = 7,
					       .reliable = false,
					       .tick_latency = 0 };
		nrfx_rtc_uninit(&rtc_timer);
		if (nrfx_rtc_init(&rtc_timer, &p_config, IDLE_TIMER_INT_HANDLER) != NRFX_SUCCESS) {
			QLOGE("qtimer: initialization failed for id %d.", qtimer_id);
			return NULL;
		}
	}
	timer_instance->handler = handler;
	timer_instance->arg = arg;
	timer_instance->timer.qtimer_id = qtimer_id;
	timer_instance->is_used = true;
	return &timer_instance->timer;
}

#define RTC_PRESCALER_TO_FREQ(PRESCALER) (uint16_t)((RTC_INPUT_FREQ) / ((PRESCALER) + 1))
enum qerr qtimer_start(const struct qtimer *timer, uint32_t us, bool periodic)
{
	if (!timer) {
		QLOGE("Timer is NULL.");
		return QERR_EINVAL;
	}
	/* If no `us` parameter, timer is a free running timer. */
	if (!us) {
		return QERR_SUCCESS;
	}
	struct qtimer_instance *timer_instance = qrtc_find_this_timer(timer->qtimer_id);
	if (!timer_instance) {
		QLOGE("No timer to start for id %d.", timer->qtimer_id);
		return QERR_EINVAL;
	}
	/* Set periodic flag. */
	timer_instance->periodic = periodic;
	/* Program an interrupt on specified user time. */
	uint32_t prescaler = rtc_timer.p_reg->PRESCALER;
	uint16_t freq = RTC_PRESCALER_TO_FREQ(prescaler);
	uint32_t old_ticks = nrfx_rtc_counter_get(&rtc_timer);
	uint32_t time_ticks = NRFX_RTC_US_TO_TICKS((uint64_t)us, (uint64_t)freq) + old_ticks;
	timer_instance->interval = NRFX_RTC_US_TO_TICKS((uint64_t)us, (uint64_t)freq);
	nrfx_rtc_cc_set(&rtc_timer, timer_instance->timer.rtc_cc_instance, time_ticks, true);

	return QERR_SUCCESS;
}

enum qerr qtimer_stop(const struct qtimer *timer)
{
	if (!timer) {
		QLOGE("Timer is NULL.");
		return QERR_EINVAL;
	}

	struct qtimer_instance *timer_instance = qrtc_find_this_timer(timer->qtimer_id);
	if (!timer_instance) {
		QLOGE("No timer to stop for id %d.", timer->qtimer_id);
		return QERR_EINVAL;
	}

	nrfx_rtc_cc_disable(&rtc_timer, timer->rtc_cc_instance);

	return QERR_SUCCESS;
}

enum qerr qtimer_read(const struct qtimer *timer, uint32_t *us)
{
	if (!timer || !us) {
		QLOGE("Timer or us is NULL.");
		return QERR_EINVAL;
	}

	uint32_t prescaler = rtc_timer.p_reg->PRESCALER;
	uint16_t freq = RTC_PRESCALER_TO_FREQ(prescaler);
	uint32_t raw_value = nrfx_rtc_counter_get(&rtc_timer);

	*us = (uint32_t)((uint64_t)raw_value * US_IN_S / freq);

	return QERR_SUCCESS;
}
