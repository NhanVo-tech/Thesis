/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <qerr.h>
#include <qgpio.h>
#include <qirq.h>
#ifdef __ZEPHYR__
#include <hal/nrf_gpio.h>
#else
#include "nrf_gpio.h"
#endif
#include "nrfx_gpiote.h"

#define LOG_TAG "qgpio_nrfx"
#include <qlog.h>

#ifndef CONFIG_QHAL_MAX_GPIO_CALLBACKS
#error "CONFIG_QHAL_MAX_GPIO_CALLBACKS must be defined with maximun number of gpio used on this firmware."
#endif

struct qgpio_cb_data {
	uint32_t abs_pin;
	qgpio_irq_cb cb;
	void *arg;
	bool in_use;
};

static struct qgpio_cb_data cb_data[CONFIG_QHAL_MAX_GPIO_CALLBACKS] = { 0 };

/**
 * Look for callback data regarding absolute pin_number.
 *
 * Return `qgpio_cb_data *`, or NULL if not found.
 */
static struct qgpio_cb_data *find_cb_data(uint32_t abs_pin)
{
	for (uint8_t idx = 0; idx < CONFIG_QHAL_MAX_GPIO_CALLBACKS; idx++) {
		if (cb_data[idx].abs_pin == abs_pin)
			return &cb_data[idx];
	}

	return NULL;
}

static enum qerr convert_to_nrfx_irq_flags(int flags, nrfx_gpiote_in_config_t *trig_config)
{
	if (!(flags & QGPIO_IRQ_MASK)) {
		QLOGE("No IRQ configuration requested");
		return QERR_EINVAL;
	}

	/* No level or edge triggering difference on nrfx. */
	switch (flags & (INTERNAL_QGPIO_INT_LOW | INTERNAL_QGPIO_INT_HIGH)) {
	case (INTERNAL_QGPIO_INT_LOW | INTERNAL_QGPIO_INT_HIGH): {
		/* Both polarities case. */
		nrfx_gpiote_in_config_t convert = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
		*trig_config = convert;
	} break;

	case INTERNAL_QGPIO_INT_LOW: {
		nrfx_gpiote_in_config_t convert = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
		*trig_config = convert;
	} break;

	case INTERNAL_QGPIO_INT_HIGH: {
		nrfx_gpiote_in_config_t convert = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
		*trig_config = convert;
	} break;

	default:
		QLOGE("Not a valid polarity value, nor rising, falling or both");
		return QERR_EINVAL;
	}

	if (flags & QGPIO_PULL_DOWN)
		trig_config->pull = NRF_GPIO_PIN_PULLDOWN;

	if (flags & QGPIO_PULL_UP)
		trig_config->pull = NRF_GPIO_PIN_PULLUP;

	return QERR_SUCCESS;
}

static void local_nrfx_gpio_handler(nrfx_gpiote_pin_t abs_pin, nrf_gpiote_polarity_t action)
{
	for (uint8_t idx = 0; idx < CONFIG_QHAL_MAX_GPIO_CALLBACKS; idx++) {
		if (cb_data[idx].abs_pin == abs_pin)
			if (cb_data[idx].cb)
				/* Call user function with arg. */
				cb_data[idx].cb(cb_data[idx].arg);
	}
}

static nrf_gpio_pin_pull_t get_nrf_gpio_pin_pull_cfg(uint32_t flags)
{
	nrf_gpio_pin_pull_t pull_flag;

	if (flags & QGPIO_PULL_UP)
		pull_flag = NRF_GPIO_PIN_PULLUP;
	else if (flags & QGPIO_PULL_DOWN)
		pull_flag = NRF_GPIO_PIN_PULLDOWN;
	else
		pull_flag = NRF_GPIO_PIN_NOPULL;

	return pull_flag;
}

static nrf_gpio_pin_drive_t get_nrf_gpio_pin_drive_cfg(uint32_t flags)
{
	if (flags & INTERNAL_QGPIO_OUTPUT_INIT_HIGH)
		return NRF_GPIO_PIN_H0H1;

	return NRF_GPIO_PIN_S0S1;
}

enum qerr qgpio_pin_configure(const struct qgpio *qgpio_pin, uint32_t flags)
{
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(qgpio_pin->port, qgpio_pin->pin_number);

	if (flags == QGPIO_DISCONNECTED) {
		nrf_gpio_cfg_default(abs_pin);
		return QERR_SUCCESS;
	}

	nrf_gpio_cfg(abs_pin,
		     (flags & INTERNAL_QGPIO_OUTPUT) ? NRF_GPIO_PIN_DIR_OUTPUT :
						       NRF_GPIO_PIN_DIR_INPUT,
		     (flags & INTERNAL_QGPIO_OUTPUT) ? NRF_GPIO_PIN_INPUT_DISCONNECT :
						       NRF_GPIO_PIN_INPUT_CONNECT,
		     get_nrf_gpio_pin_pull_cfg(flags), get_nrf_gpio_pin_drive_cfg(flags),
		     NRF_GPIO_PIN_NOSENSE);

	if (flags & INTERNAL_QGPIO_OUTPUT_INIT_HIGH)
		nrf_gpio_pin_set(abs_pin);

	if (flags & INTERNAL_QGPIO_OUTPUT_INIT_LOW)
		nrf_gpio_pin_clear(abs_pin);

	return QERR_SUCCESS;
}

enum qerr qgpio_pin_write(const struct qgpio *qgpio_pin, uint8_t value)
{
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(qgpio_pin->port, qgpio_pin->pin_number);

	if (nrf_gpio_pin_dir_get(abs_pin) != NRF_GPIO_PIN_DIR_OUTPUT) {
		return QERR_EINVAL;
	}

	if (!!value)
		nrf_gpio_pin_set(abs_pin);
	else
		nrf_gpio_pin_clear(abs_pin);

	return QERR_SUCCESS;
}

enum qerr qgpio_pin_read(const struct qgpio *qgpio_pin, uint8_t *value)
{
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(qgpio_pin->port, qgpio_pin->pin_number);

	if (!value)
		return QERR_EINVAL;

	if (nrf_gpio_pin_read(abs_pin)) {
		*value = 1;
	} else {
		*value = 0;
	}
	return QERR_SUCCESS;
}

enum qerr qgpio_pin_irq_configure(const struct qgpio *qgpio_pin, uint32_t flags)
{
	nrfx_err_t r;
	enum qerr err;
	struct qgpio_cb_data *cb_data;
	nrfx_gpiote_in_config_t trigger_config;
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(qgpio_pin->port, qgpio_pin->pin_number);

	/* Init GPIOTE at least once. */
	if (!nrfx_gpiote_is_init()) {
#ifdef NRFX_GPIOTE_IRQ_LVL_REQUIRED
		r = nrfx_gpiote_init(0);
#else
		r = nrfx_gpiote_init();
#endif
		if (r)
			return QERR_EBUSY;
	}

	if (flags & QGPIO_IRQ_DISABLED) {
		nrfx_gpiote_in_event_disable(abs_pin);
		nrfx_gpiote_in_uninit(abs_pin);
		return QERR_SUCCESS;
	}

	/* Look for struct qgpio_cb_data for this qgpio. */
	cb_data = find_cb_data(abs_pin);
	if (!cb_data) {
		QLOGW("No callback data for QGPIO pin %d, call qgpio_pin_irq_set_callback()",
		      qgpio_pin->pin_number);
		return QERR_ENODEV;
	}

	if (nrf_gpio_pin_dir_get(abs_pin) == NRF_GPIO_PIN_DIR_OUTPUT)
		return QERR_EINVAL;

	err = convert_to_nrfx_irq_flags(flags, &trigger_config);
	if (err)
		return err;

	r = nrfx_gpiote_in_init(abs_pin, &trigger_config, local_nrfx_gpio_handler);
	if (r)
		return QERR_ENODEV;

	nrfx_gpiote_in_event_enable(abs_pin, true);

	return QERR_SUCCESS;
}

enum qerr qgpio_pin_irq_set_callback(const struct qgpio *qgpio_pin, qgpio_irq_cb cb, void *arg)
{
	struct qgpio_cb_data *cb_data_cur;
	uint32_t abs_pin = NRF_GPIO_PIN_MAP(qgpio_pin->port, qgpio_pin->pin_number);

	if (nrf_gpio_pin_dir_get(abs_pin) == NRF_GPIO_PIN_DIR_OUTPUT)
		return QERR_EINVAL;

	/* Check if a callback is already registered for that pin. */
	cb_data_cur = find_cb_data(abs_pin);
	if (!cb_data_cur) {
		/* Find free slot to store cb data. */
		uint8_t idx;
		for (idx = 0; idx < CONFIG_QHAL_MAX_GPIO_CALLBACKS; idx++) {
			if (!cb_data[idx].in_use) {
				cb_data_cur = &cb_data[idx];
				break;
			}
		}
	}

	if (!cb_data_cur) {
		QLOGE("QGPIO no more available holder for callback, check CONFIG_QHAL_MAX_GPIO_CALLBACKS.");
		nrfx_gpiote_in_event_disable(abs_pin);
		return QERR_ENODEV;
	}

	cb_data_cur->abs_pin = abs_pin;
	cb_data_cur->cb = cb;
	cb_data_cur->arg = arg;
	cb_data_cur->in_use = true;

	return QERR_SUCCESS;
}
