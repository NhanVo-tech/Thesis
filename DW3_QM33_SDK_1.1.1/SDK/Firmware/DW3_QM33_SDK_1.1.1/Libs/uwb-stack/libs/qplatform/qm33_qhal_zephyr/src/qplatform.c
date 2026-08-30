/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qplatform_internal.h"

#define LOG_TAG "qplatform"
#include "deca_interface.h"
#include "qlog.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#if !DT_NODE_HAS_STATUS(DT_NODELABEL(dw35720), okay)
#error "DW3572 SPI device must be declared and enabled on device tree definitiions with dw35720 as node label"
#endif

/* Retrieve GPIO and SPI information from the device tree. */
static const struct gpio_dt_spec rstn_pin_spec =
	GPIO_DT_SPEC_GET(DT_NODELABEL(dw35720), rstn_gpios);

static const struct gpio_dt_spec irq_pin_spec = GPIO_DT_SPEC_GET(DT_NODELABEL(dw35720), irq_gpios);

static const struct device *const qm33_spi_device = DEVICE_DT_GET(DT_BUS(DT_NODELABEL(dw35720)));
static const struct spi_config qm33_spi_config =
	SPI_CONFIG_DT(DT_NODELABEL(dw35720), QSPI_OP_FLAGS, 0);

/* QM33 Reset GPIO. */
const struct qgpio qm33_rstn = {
	.dev = (void *)&rstn_pin_spec,
	.port = 0,
	.pin_number = 0,
};

/* QM33 IRQ GPIO. */
const struct qgpio qm33_irq = {
	.dev = (void *)&irq_pin_spec,
	.port = 0,
	.pin_number = 0,
};

/* QM33 SPI. */
const struct qspi_instance qm33_qspi_instance = {
	.instance_number = CONFIG_UWB_SPI_INSTANCE,
	.dev = qm33_spi_device,
};

struct qspi_config qm33_qspi_config = {
	.sck_pin = {
		.port = 0,
		.pin_number = 0,
		.dev = NULL, /* SCK pin is manaqed internally by Zephyr, no need to set a dev. */
	},
	.mosi_pin = {
		.port = 0,
		.pin_number = 0,
		.dev = NULL, /* MOSI pin is manaqed internally by Zephyr, no need to set a dev. */
	},
	.miso_pin = {
		.port = 0,
		.pin_number = 0,
		.dev = NULL, /* MISO pin is manaqed internally by Zephyr, no need to set a dev. */
	},
	.cs_pin = {
		.port = 0,
		.pin_number = 0,
		.dev = (void *)&qm33_spi_config.cs->gpio,
	},
	.freq_hz = CONFIG_SPI_UWB_SLOW_RATE_FREQ, /* Start with slow rate freq. */
	.irq_priority = CONFIG_SPI_UWB_IRQ_PRIORITY,
	.op_flags = QSPI_OP_FLAGS,
};
