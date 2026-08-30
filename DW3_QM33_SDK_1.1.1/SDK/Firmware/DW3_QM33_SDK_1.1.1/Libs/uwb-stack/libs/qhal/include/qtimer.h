/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qerr.h>
#include <stdbool.h>
#include <stdint.h>

#define US_IN_S 1000000UL

/* struct qtimer: timer descriptor (opaque). */
struct qtimer;

/*
 * enum qtimer_width - Timer bit width.
 * @QTIMER_WIDTH_8_BIT: Timer bit width 8 bit..
 * @QTIMER_WIDTH_16_BIT: Timer bit width 16 bit..
 * @QTIMER_WIDTH_24_BIT: Timer bit width 24 bit..
 * @QTIMER_WIDTH_32_BIT: Timer bit width 32 bit.
 */
enum qtimer_width {
	QTIMER_WIDTH_8_BIT = 0,
	QTIMER_WIDTH_16_BIT,
	QTIMER_WIDTH_24_BIT,
	QTIMER_WIDTH_32_BIT
};

/**
 * struct qtimer_config - Timer configuration.
 *
 * @freq_hz: Timer frequency in Hz.
 * @irq_priority: Timer IRQ priority.
 * @width:Timer bit width.
 */
struct qtimer_config {
	uint32_t freq_hz;
	uint8_t irq_priority;
	enum qtimer_width width;
};

/**
 * typedef qtimer_cb - Pointer to a timer callback.
 * @arg: private data of the timer.
 *
 * Return: nothing.
 */
typedef void (*qtimer_cb)(void *arg);

/**
 * qtimer_init() - Initialize and configure a TIMER device.
 *
 * NOTE: This function should be called first.
 *
 * @qtimer_id: ID of the timer. Implementation and hardware dependent.
 * @config: Pointer to timer configuration.
 * @handler: Callback to called when timer ends.
 * @arg: Private data of the timer callback.
 *
 * Return: Pointer to the initialized timer descriptor.
 */
struct qtimer *qtimer_init(uint8_t qtimer_id, const struct qtimer_config *config, qtimer_cb handler,
			   void *arg);

/**
 * qtimer_start() - Start a timer.
 * @timer: Pointer to the timer descriptor.
 * @us: Number of microseconds until timer expiration.
 * @periodic: true for a one-shot timer, false for a cyclic timer.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qtimer_start(const struct qtimer *timer, uint32_t us, bool periodic);

/**
 * qtimer_stop() - Start a timer.
 * @timer: Pointer to the timer descriptor.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qtimer_stop(const struct qtimer *timer);

/**
 * qtimer_read() - Read qtimer value.
 * @timer: Pointer to the timer descriptor.
 * @us: Elapsed timer in µs.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qtimer_read(const struct qtimer *timer, uint32_t *us);
