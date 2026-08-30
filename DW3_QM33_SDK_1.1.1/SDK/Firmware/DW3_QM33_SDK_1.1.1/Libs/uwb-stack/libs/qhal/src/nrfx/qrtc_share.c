/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */
#include "qrtc_share.h"

#include <qlog.h>

#define RTC_MAX_SUBSCRIBER_COUNT RTC2_CC_NUM

extern void rtc_overflow_handler(void);
nrfx_rtc_t rtc_timer = NRFX_RTC_INSTANCE(IDLE_TIMER_RTC_INSTANCE);
static struct qtimer_instance local_instance_timer[RTC_MAX_SUBSCRIBER_COUNT] = { 0 };

uint8_t get_local_instance_idx(uint8_t rtc_cc_number)
{
	uint8_t idx;
	for (idx = 0; idx < RTC_MAX_SUBSCRIBER_COUNT; idx++) {
		if (local_instance_timer[idx].timer.rtc_cc_instance == rtc_cc_number) {
			return idx;
		}
	}
	return RTC_MAX_SUBSCRIBER_COUNT;
}

struct qtimer_instance *qrtc_find_this_timer(uint8_t qtimer_id)
{
	uint8_t idx;

	for (idx = 0; idx < RTC_MAX_SUBSCRIBER_COUNT; idx++) {
		if (local_instance_timer[idx].timer.qtimer_id == qtimer_id &&
		    local_instance_timer[idx].is_used) {
			break;
		}
	}

	return idx < RTC_MAX_SUBSCRIBER_COUNT ? &local_instance_timer[idx] : NULL;
}

struct qtimer_instance *qrtc_find_available_timer(void)
{
	uint8_t idx;

	for (idx = 0; idx < RTC_MAX_SUBSCRIBER_COUNT; idx++) {
		if (!local_instance_timer[idx].is_used) {
			break;
		}
	}

	return idx < RTC_MAX_SUBSCRIBER_COUNT ? &local_instance_timer[idx] : NULL;
}

void qrtc_handler_shared(nrfx_rtc_int_type_t int_type)
{
	uint32_t current_count;
	if (int_type == NRFX_RTC_INT_OVERFLOW)
		rtc_overflow_handler();
	else {
		uint8_t idx = get_local_instance_idx(int_type);
		if (idx < RTC_MAX_SUBSCRIBER_COUNT) {
			local_instance_timer[idx].handler(local_instance_timer[idx].arg);
			/* Enable RTC interrupt if periodic */
			if (local_instance_timer[idx].periodic) {
				current_count = nrfx_rtc_counter_get(&rtc_timer);
				uint32_t next_compare =
					(current_count + local_instance_timer[idx].interval) &
					RTC_MAX_COUNT_24BITS;
				nrfx_rtc_cc_set(&rtc_timer, (uint32_t)int_type, next_compare, true);
			} else {
				nrfx_rtc_cc_disable(&rtc_timer, (uint32_t)int_type);
			}
		} else {
			QLOGE("Invalid RTC instance idx %d", idx);
		}
	}
}
