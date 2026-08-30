/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <device.h>
#include <drivers/gpio.h>
#include <qerr.h>
#include <qgpio.h>
#include <version.h>
#include <zephyr.h>

#define LOG_TAG "qgpio_zephyr"
#include <qlog.h>

#ifndef CONFIG_QHAL_MAX_GPIO_CALLBACKS
#error "CONFIG_QHAL_MAX_GPIO_CALLBACKS must be defined with maximun number of gpio used on this firmware."
#endif

struct qgpio_cb_data {
	const struct gpio_dt_spec *dev;
	struct gpio_callback zephyr_gpio_cb;
	qgpio_irq_cb cb;
	void *arg;
};

static struct qgpio_cb_data cb_data[CONFIG_QHAL_MAX_GPIO_CALLBACKS] = { 0 };

/**
 * Validate that GPIO port is ready.
 *
 * @param spec GPIO specification from devicetree
 *
 * Return: true if the GPIO spec is ready for use.
 *         false if the GPIO spec is not ready for use.
 */
static inline bool gpio_is_ready_dt(const struct gpio_dt_spec *spec)
{
	/* Validate port is ready */
	return device_is_ready(spec->port);
}

/**
 * Add an application callback.
 *
 * This is equivalent to:
 *
 *     gpio_add_callback(spec->port, callback);
 *
 * @param spec GPIO specification from devicetree.
 * @param callback A valid application's callback structure pointer.
 *
 * Return: value from gpio_add_callback().
 */
static inline int gpio_add_callback_dt(const struct gpio_dt_spec *spec,
				       struct gpio_callback *callback)
{
	return gpio_add_callback(spec->port, callback);
}

/**
 * Get Zephyr GPIO device.
 *
 * Return QERR_SUCCES on success, enum qerr otherwise.
 */
static enum qerr get_zephyr_device(const struct qgpio *qgpio, struct gpio_dt_spec **dev)
{
	if (!qgpio || !qgpio->dev || !dev)
		return QERR_EINVAL;

	*dev = (struct gpio_dt_spec *)qgpio->dev;

	if (!gpio_is_ready_dt(*dev)) {
		QLOGW("QGPIO device not ready for %d pin", qgpio->pin_number);
		return QERR_ENODEV;
	}

	return QERR_SUCCESS;
}

/**
 * Look for callback data regarding qgpio dt_spec.
 *
 * Return `qgpio_cb_data *`, or NULL if not found.
 */
static struct qgpio_cb_data *find_cb_data(const struct gpio_dt_spec *dev)
{
	int idx;

	for (idx = 0; idx < CONFIG_QHAL_MAX_GPIO_CALLBACKS; idx++) {
		if (cb_data[idx].dev->port == dev->port && cb_data[idx].dev->pin == dev->pin)
			return &cb_data[idx];
	}

	return NULL;
}

static enum qerr convert_to_zephyr_gpio_flags(uint32_t flags, gpio_flags_t *zephyr_flags)
{
	if (flags & QGPIO_INPUT) {
		*zephyr_flags |= GPIO_INPUT;
	}
	if (flags & QGPIO_PULL_UP)
		*zephyr_flags |= GPIO_PULL_UP;

	if (flags & QGPIO_PULL_DOWN)
		*zephyr_flags |= GPIO_PULL_DOWN;

	if (flags & QGPIO_OUTPUT_LOW)
		*zephyr_flags |= GPIO_OUTPUT_LOW;

	if (flags & QGPIO_OUTPUT_HIGH)
		*zephyr_flags |= GPIO_OUTPUT_HIGH;

	return QERR_SUCCESS;
}

static enum qerr convert_to_zephyr_irq_flags(uint32_t flags, gpio_flags_t *zephyr_flags)
{
	if (!(flags & QGPIO_IRQ_MASK)) {
		QLOGE("No IRQ configuration requested");
		return QERR_EINVAL;
	}

	if (flags & QGPIO_IRQ_DISABLED) {
		*zephyr_flags |= GPIO_INT_DISABLE;

		return QERR_SUCCESS;
	}

	if (flags & INTERNAL_QGPIO_INT_EDGE) {
		*zephyr_flags |= GPIO_INT_ENABLE;

		switch (flags & QGPIO_IRQ_EDGE_BOTH) {
		case QGPIO_IRQ_EDGE_BOTH:
			*zephyr_flags |= GPIO_INT_EDGE_BOTH;
			break;
		case QGPIO_IRQ_EDGE_RISING:
			*zephyr_flags |= GPIO_INT_EDGE_RISING;
			break;
		case QGPIO_IRQ_EDGE_FALLING:
			*zephyr_flags |= GPIO_INT_EDGE_FALLING;
			break;
		default:
			QLOGE("Not a valid trigger value, nor rising, falling or both");
			return QERR_EINVAL;
		}
	} else {
		switch (flags & (QGPIO_IRQ_LEVEL_LOW | QGPIO_IRQ_LEVEL_HIGH)) {
		case QGPIO_IRQ_LEVEL_LOW:
			*zephyr_flags |= GPIO_INT_LEVEL_LOW;
			break;
		case QGPIO_IRQ_LEVEL_HIGH:
			*zephyr_flags |= GPIO_INT_LEVEL_HIGH;
			break;
		default:
			QLOGE("Not a valid level trigger value");
			return QERR_EINVAL;
		}
	}

	return QERR_SUCCESS;
}

static void generic_callback(const struct device *dev, struct gpio_callback *callback,
			     uint32_t pins)
{
	struct qgpio_cb_data *gpio_data =
		CONTAINER_OF(callback, struct qgpio_cb_data, zephyr_gpio_cb);

	if (gpio_data->cb)
		gpio_data->cb(gpio_data->arg);
}

enum qerr qgpio_pin_configure(const struct qgpio *qgpio_pin, uint32_t flags)
{
	int ret;
	gpio_flags_t zephyr_gpio_flags = 0;
	struct gpio_dt_spec *dev;
	enum qerr r;

	/* Get GPIO device. */
	r = get_zephyr_device(qgpio_pin, &dev);
	if (r)
		return r;

	r = convert_to_zephyr_gpio_flags(flags, &zephyr_gpio_flags);
	if (r) {
		QLOGW("Wrong GPIO flags for QGPIO pin %d, error %d", dev->pin, r);
		return r;
	}

	/* Configure the GPIO pin. */
	ret = gpio_pin_configure_dt(dev, zephyr_gpio_flags);
	if (ret) {
		QLOGW("Failed to configure QGPIO pin %d", dev->pin);
		return qerr_convert_os_to_qerr(ret);
	}

	return QERR_SUCCESS;
}

enum qerr qgpio_pin_write(const struct qgpio *qgpio_pin, uint8_t value)
{
	int ret;
	struct gpio_dt_spec *dev;
	enum qerr r;

	/* Get GPIO device. */
	r = get_zephyr_device(qgpio_pin, &dev);
	if (r)
		return r;

#if (KERNEL_VERSION_MAJOR >= 3) && (KERNEL_VERSION_MINOR >= 1)
	ret = gpio_pin_set_dt(dev, !!value);
#else
	ret = gpio_pin_set(dev->port, dev->pin, !!value);
#endif
	if (ret < 0) {
		QLOGW("Failed to write QGPIO pin %d, error %d", dev->pin, ret);
		return qerr_convert_os_to_qerr(ret);
	}

	return QERR_SUCCESS;
}

enum qerr qgpio_pin_read(const struct qgpio *qgpio_pin, uint8_t *value)
{
	int ret;
	struct gpio_dt_spec *dev;
	enum qerr r;

	/* Get GPIO device. */
	r = get_zephyr_device(qgpio_pin, &dev);
	if (r)
		return r;

	if (!value)
		return QERR_EINVAL;

	*value = 0;

#if (KERNEL_VERSION_MAJOR >= 3) && (KERNEL_VERSION_MINOR >= 1)
	ret = gpio_pin_get_dt(dev);
#else
	ret = gpio_pin_get(dev->port, dev->pin);
#endif
	if (ret < 0) {
		QLOGW("Failed to read QGPIO pin %d, error %d", dev->pin, ret);
		return qerr_convert_os_to_qerr(ret);
	}

	*value = !!ret;

	return QERR_SUCCESS;
}

enum qerr qgpio_pin_irq_configure(const struct qgpio *qgpio_pin, uint32_t flags)
{
	int ret;
	gpio_flags_t zephyr_irq_flags = 0;
	struct gpio_dt_spec *dev;
	struct qgpio_cb_data *cb_data;
	enum qerr r;

	/* Get GPIO device. */
	r = get_zephyr_device(qgpio_pin, &dev);
	if (r)
		return r;

	/* Look for struct qgpio_cb_data for this qgpio. */
	cb_data = find_cb_data(dev);
	if (!cb_data) {
		QLOGW("No callback data for QGPIO pin %d, call qgpio_pin_irq_set_callback()",
		      dev->pin);
		return QERR_ENODEV;
	}

	gpio_init_callback(&cb_data->zephyr_gpio_cb, generic_callback, BIT(cb_data->dev->pin));
	ret = gpio_add_callback_dt(dev, &cb_data->zephyr_gpio_cb);
	if (ret < 0) {
		QLOGW("Could not install IRQ callback for QGPIO pin %d, error %d", dev->pin, ret);
		return qerr_convert_os_to_qerr(ret);
	}

	r = convert_to_zephyr_irq_flags(flags, &zephyr_irq_flags);
	if (r) {
		QLOGW("Wrong IRQ flags for QGPIO pin %d, error %d", dev->pin, r);
		return r;
	}

	ret = gpio_pin_interrupt_configure_dt(dev, zephyr_irq_flags);
	if (ret < 0) {
		QLOGW("Failed to configure IRQ for QGPIO pin %d, error %d", dev->pin, ret);
		return qerr_convert_os_to_qerr(ret);
	}

	return QERR_SUCCESS;
}

enum qerr qgpio_pin_irq_set_callback(const struct qgpio *qgpio_pin, qgpio_irq_cb cb, void *arg)
{
	/* Look for a free struct qgpio_cb_data. */
	int idx;
	bool found = false;
	struct gpio_dt_spec *dev;
	enum qerr r;

	/* Get GPIO device. */
	r = get_zephyr_device(qgpio_pin, &dev);
	if (r)
		return r;

	for (idx = 0; idx < CONFIG_QHAL_MAX_GPIO_CALLBACKS; idx++) {
		const struct gpio_dt_spec *cb_dev = cb_data[idx].dev;

		if (!cb_dev || (cb_dev->port == dev->port && cb_dev->pin == dev->pin)) {
			found = true;
			break;
		}
	}

	if (!found) {
		QLOGW("QGPIO no more available holder for callback, check CONFIG_QHAL_MAX_GPIO_CALLBACKS.");
		return QERR_ENODEV;
	}

	cb_data[idx].dev = (const struct gpio_dt_spec *)qgpio_pin->dev;
	cb_data[idx].cb = cb;
	cb_data[idx].arg = arg;

	return QERR_SUCCESS;
}
