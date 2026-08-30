/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#ifndef BIT
/*
 * BIT - Set a bit to 1.
 * @n: bit number.
 * Return: integer with bit n set to 1.
 */
#define BIT(n) (1ul << (n))
#endif

#ifndef BIT_TO_0
/*
 * BIT_TO_0 - Set a bit to 0. Macro added for API readability.
 * @n: bit number.
 * Return: integer with bit n set to 0 (so 0).
 */
#define BIT_TO_0(n) (0ul << (n))
#endif

#include <qerr.h>
#include <stdint.h>

/**
 * struct qgpio - Descriptor of a GPIO pin.
 * @pin_number: pin number.
 * @port: port/bank for this GPIO.
 * @dev: User pointer to pass device structure used by HAL.
 */
struct qgpio {
	uint32_t pin_number;
	uint32_t port;
	void *dev;
};

/*
 *  GPIO configuration flags bitfield.
 *     operational mode    [ 0 ]      - single-ended or push-pull mode
 *     single ended conf.  [ 1 ]      - Open Drain or Open Source
 *     pin pull up         [ 2 ]      - Pull Up output.
 *     pin pull down       [ 3 ]      - Pull Down output.
 *     slew rate conf.     [ 4 : 5 ]  - GPIO rise and fall time.
 *     RFU		   [ 6 : 7 ]  - Reserved for future use.
 *     input direction     [ 8 ]      - GPIO as input (0) / output (1).
 *     RFU                 [ 9 ]      - Reserved for future use.
 *     initial state low   [ 10 ]     - Set initial GPIO state as low.
 *     initial state high  [ 11 ]     - Set initial GPIO state as high.
 *     GPIO irq disabled   [ 12 ]     - Disable interrupt on GPIO.
 *     GPIO irq enabled    [ 13 ]     - Enable interrupt on GPIO.
 *     Edge sensitive      [ 14 ]     - Enable edge sensing.
 *     Detection low       [ 15 ]     - Either falling edge on low state.
 *     Detection high      [ 16 ]     - Either rising edge on high state.
 */

/** QGPIO_DISCONNECTED - Disable pin for both input and output. Whole flags variable, not a
 * bitfield. */
#define QGPIO_DISCONNECTED 0

/**
 * QGPIO_SINGLE_ENDED - Configure GPIO output in single-ended mode (open drain or open source).
 */
#define QGPIO_SINGLE_ENDED BIT(0)

/**
 * QGPIO_PUSH_PULL - Configure GPIO output in push-pull mode
 */
#define QGPIO_PUSH_PULL BIT_TO_0(0)

/**
 * QGPIO_LINE_OPEN_DRAIN - Configure single ended open drain mode (wired AND).
 */
#define QGPIO_LINE_OPEN_DRAIN BIT(1)

/**
 * QGPIO_LINE_OPEN_SOURCE - Configure single ended open source mode (wired OR).
 */
#define QGPIO_LINE_OPEN_SOURCE BIT_TO_0(1)

/**
 * QGPIO_OPEN_DRAIN - Configure GPIO output in open drain mode (wired AND).
 */
#define QGPIO_OPEN_DRAIN (QGPIO_SINGLE_ENDED | QGPIO_LINE_OPEN_DRAIN)

/**
 * QGPIO_OPEN_SOURCE - Configure GPIO output in open source mode (wired OR).
 */
#define QGPIO_OPEN_SOURCE (QGPIO_SINGLE_ENDED | QGPIO_LINE_OPEN_SOURCE)

/**
 * QGPIO_PULL_UP - Configure GPIO pin pull-up.
 */
#define QGPIO_PULL_UP BIT(2)

/**
 * QGPIO_PULL_DOWN - Configure GPIO pin pull-down.
 */
#define QGPIO_PULL_DOWN BIT(3)

/* Configure GPIO slew rate (the rise time and fall time) of the output
 * signal.*/

/**
 * QGPIO_SPEED_2MHZ - Configure GPIO output slew rate to 2 MHz.
 */
#define QGPIO_SPEED_2MHZ (BIT_TO_0(4) | BIT_TO_0(5))

/**
 * QGPIO_SPEED_25MHZ - Configure GPIO output slew rate to 25 MHz.
 */
#define QGPIO_SPEED_25MHZ (BIT(4) | BIT_TO_0(5))

/**
 * QGPIO_SPEED_50MHZ - Configure GPIO output slew rate to 50 MHz.
 */
#define QGPIO_SPEED_50MHZ (BIT_TO_0(4) | BIT(5))

/**
 * QGPIO_SPEED_100MHZ - Configure GPIO output slew rate to 100 MHz.
 */
#define QGPIO_SPEED_100MHZ (BIT(4) | BIT(5))

/* Internal definitions. */
/* Enable pin as output, no change to the output state. */
#define INTERNAL_QGPIO_OUTPUT BIT(8)
/* Initialize output to a low state. */
#define INTERNAL_QGPIO_OUTPUT_INIT_LOW BIT(10)
/* Initialize output to a high state. */
#define INTERNAL_QGPIO_OUTPUT_INIT_HIGH BIT(11)

/**
 * QGPIO_INPUT - Enable pin as input.
 */
#define QGPIO_INPUT BIT(9)

/**
 * QGPIO_DIR_MASK - GPIO direction mask.
 */
#define QGPIO_DIR_MASK (QGPIO_INPUT | INTERNAL_QGPIO_OUTPUT)

/**
 * QGPIO_OUTPUT_LOW - Configure GPIO pin as output and initializes it to a low state.
 */
#define QGPIO_OUTPUT_LOW (INTERNAL_QGPIO_OUTPUT | INTERNAL_QGPIO_OUTPUT_INIT_LOW)

/**
 * QGPIO_OUTPUT_HIGH - Configure GPIO pin as output and initializes it to a high state.
 */
#define QGPIO_OUTPUT_HIGH (INTERNAL_QGPIO_OUTPUT | INTERNAL_QGPIO_OUTPUT_INIT_HIGH)

/* Internal definitions. */
/* Disable GPIO pin interrupt. */
#define INTERNAL_QGPIO_INT_DISABLE BIT(12)
/* Enable GPIO pin interrupt. */
#define INTERNAL_QGPIO_INT_ENABLE BIT(13)
/* GPIO interrupt is edge sensitive. */
#define INTERNAL_QGPIO_INT_EDGE BIT(14)
/* Trigger detection when input state is (or transitions to) physical low. */
#define INTERNAL_QGPIO_INT_LOW BIT(15)
/* Trigger detection on input state is (or transitions to) physical high. */
#define INTERNAL_QGPIO_INT_HIGH BIT(16)

/**
 * QGPIO_IRQ_EDGE_RISING - Configure GPIO interrupt to be triggered on pin rising edge and enables
 * it.
 */
#define QGPIO_IRQ_EDGE_RISING \
	(INTERNAL_QGPIO_INT_ENABLE | INTERNAL_QGPIO_INT_EDGE | INTERNAL_QGPIO_INT_HIGH)

/**
 * QGPIO_IRQ_EDGE_FALLING - Configure GPIO interrupt to be triggered on pin falling edge and
 * enables it.
 */
#define QGPIO_IRQ_EDGE_FALLING \
	(INTERNAL_QGPIO_INT_ENABLE | INTERNAL_QGPIO_INT_EDGE | INTERNAL_QGPIO_INT_LOW)

/**
 * QGPIO_IRQ_EDGE_BOTH - Configure GPIO interrupt to be triggered on pin rising or falling edge and
 * enables it.
 */
#define QGPIO_IRQ_EDGE_BOTH                                                             \
	(INTERNAL_QGPIO_INT_ENABLE | INTERNAL_QGPIO_INT_EDGE | INTERNAL_QGPIO_INT_LOW | \
	 INTERNAL_QGPIO_INT_HIGH)

/**
 * QGPIO_IRQ_LEVEL_LOW - Configure GPIO interrupt to be triggered on pin physical level low and
 * enables it.
 */
#define QGPIO_IRQ_LEVEL_LOW (INTERNAL_QGPIO_INT_ENABLE | INTERNAL_QGPIO_INT_LOW)

/**
 * QGPIO_IRQ_LEVEL_HIGH - Configure GPIO interrupt to be triggered on pin physical level high and
 * enables it.
 */
#define QGPIO_IRQ_LEVEL_HIGH (INTERNAL_QGPIO_INT_ENABLE | INTERNAL_QGPIO_INT_HIGH)

/**
 * QGPIO_IRQ_MASK - GPIO interrupt mask.
 */
#define QGPIO_IRQ_MASK                                                                      \
	(INTERNAL_QGPIO_INT_DISABLE | INTERNAL_QGPIO_INT_ENABLE | INTERNAL_QGPIO_INT_EDGE | \
	 INTERNAL_QGPIO_INT_LOW | INTERNAL_QGPIO_INT_HIGH)

/**
 * QGPIO_IRQ_DISABLED - Disable QGPIO IRQ.
 */
#define QGPIO_IRQ_DISABLED INTERNAL_QGPIO_INT_DISABLE

/**
 * typedef qgpio_irq_cb - Pointer to a callback called when interrupt triggered.
 * NOTE: Considered this called from ISR context unless indicated otherwise on implementation.
 * @arg: private data of the GPIO interruption handler.
 *
 * Return: nothing.
 */
typedef void (*qgpio_irq_cb)(void *arg);

/**
 * qgpio_pin_configure() - Configure a GPIO.
 *
 * NOTE: To be called before any others functions of qgpio.
 *
 * @qgpio_pin: Pin descriptor.
 * @flags: Pin configuration flags.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qgpio_pin_configure(const struct qgpio *qgpio_pin, uint32_t flags);

/**
 * qgpio_pin_write() - Write a GPIO.
 * @qgpio_pin: Pin descriptor.
 * @value: Value to be written.
 *
 * Return: QERR_SUCCESS or error.
 *
 * NOTE: To be called after qgpio_pin_configure().
 */
enum qerr qgpio_pin_write(const struct qgpio *qgpio_pin, uint8_t value);

/**
 * qgpio_pin_read() - Read a GPIO.
 * @qgpio_pin: Pin descriptor.
 * @value: Pointer to value being read.
 *
 * Return: QERR_SUCCESS or error.
 *
 * NOTE: To be called after qgpio_pin_configure().
 */
enum qerr qgpio_pin_read(const struct qgpio *qgpio_pin, uint8_t *value);

/**
 * qgpio_pin_irq_configure() - Configure interrupt for GPIO.
 *
 * @qgpio_pin: Pin descriptor.
 * @flags: Interrupt pin configuration flags.
 *
 * Return: QERR_SUCCESS or error.
 *
 * NOTE: To be called after qgpio_pin_irq_set_callback().
 */
enum qerr qgpio_pin_irq_configure(const struct qgpio *qgpio_pin, uint32_t flags);

/**
 * qgpio_pin_irq_set_callback() - Set callback function to be called when GPIO
 * interrupt.
 * @qgpio_pin: Pin descriptor.
 * @cb: Callback function pointer to be called in case of interrupt.
 * @arg: Private data for callback.
 *
 * Return: QERR_SUCCESS or error.
 *
 * NOTE: To be called after qgpio_pin_configure() and before qgpio_pin_irq_configure().
 */
enum qerr qgpio_pin_irq_set_callback(const struct qgpio *qgpio_pin, qgpio_irq_cb cb, void *arg);
