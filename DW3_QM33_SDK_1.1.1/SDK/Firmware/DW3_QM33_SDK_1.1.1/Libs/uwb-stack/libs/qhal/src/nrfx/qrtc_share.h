/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#ifndef QRTC_SHARE_H
#define QRTC_SHARE_H

#include <nrfx_rtc.h>
#include <qtimer.h>

#define RTC_MAX_COUNT_24BITS (0xFFFFFF)

#define BASE_TIMER_RTC_INSTANCE 2
#define IDLE_TIMER_RTC_INSTANCE 2 // Using RTC2 for idle timer

#if BASE_TIMER_RTC_INSTANCE == IDLE_TIMER_RTC_INSTANCE
#define RTC_INSTANCE_SHARED 1
#else
#define RTC_INSTANCE_SHARED 0
#endif

struct qtimer {
	uint8_t rtc_cc_instance; /* RTC rtc_cc_instance. */
	uint8_t qtimer_id; /* qtimer_id. */
};

struct qtimer_instance {
	struct qtimer timer; /* qtimer, which contains rtc_cc_instance and qtimer_id. */
	qtimer_cb handler; /* User qtimer handler. */
	void *arg; /* User qtimer handler argument. */
	uint32_t interval; /* Timer interval in ticks. */
	bool is_used; /* Flag to check if the timer is used. */
	bool periodic; /* Flag to check if the timer is periodic. */
};

/**
 * qrtc_find_available_timer() - function to find a timer which is not used.
 *
 * Return: reference to qtimer_instance.
 */
struct qtimer_instance *qrtc_find_available_timer(void);

/**
 * qrtc_find_this_timer() - function to find a timer which was previously claimed.
 * @qtimer_id: qtimer_id.
 *
 * Return: reference to qtimer_instance.
 */
struct qtimer_instance *qrtc_find_this_timer(uint8_t qtimer_id);

/**
 * qrtc2_handler_shared() - function to handle RTC2 interrupt.
 * @int_type: Interrupt type.
 *
 * Return: Nothing.
 */
void qrtc_handler_shared(nrfx_rtc_int_type_t int_type);

#define IDLE_TIMER_INT_HANDLER qrtc_handler_shared
#define BASE_TIMER_INT_HANDLER qrtc_handler_shared

#endif /* QRTC_SHARE_H */
