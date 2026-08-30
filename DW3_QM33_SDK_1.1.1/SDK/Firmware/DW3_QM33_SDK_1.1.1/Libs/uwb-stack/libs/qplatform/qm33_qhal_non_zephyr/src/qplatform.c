/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qplatform_internal.h"

#define LOG_TAG "qplatform"
#include "deca_interface.h"
#include "qlog.h"

/* QM33 Reset GPIO. */
const struct qgpio qm33_rstn = {
	.dev = NULL,
	.port = CONFIG_DWT_RSTN_GPIO_PORT,
	.pin_number = CONFIG_DWT_RSTN_GPIO_PIN,
};

/* QM33 IRQ GPIO. */
const struct qgpio qm33_irq = {
	.dev = NULL,
	.port = CONFIG_DWT_IRQ_GPIO_PORT,
	.pin_number = CONFIG_DWT_IRQ_GPIO_PIN,
};

/* QM33 SPI. */
const struct qspi_instance qm33_qspi_instance = {
	.instance_number = CONFIG_UWB_SPI_INSTANCE,
	.dev = NULL,
};

struct qspi_config qm33_qspi_config = {
	.sck_pin = {
		.port = CONFIG_SPI_UWB_SCK_GPIO_PORT,
		.pin_number = CONFIG_SPI_UWB_SCK_GPIO_PIN,
		.dev = NULL,
	},
	.mosi_pin = {
		.port = CONFIG_SPI_UWB_MOSI_GPIO_PORT,
		.pin_number = CONFIG_SPI_UWB_MOSI_GPIO_PIN,
		.dev = NULL,
	},
	.miso_pin = {
		.port = CONFIG_SPI_UWB_MISO_GPIO_PORT,
		.pin_number = CONFIG_SPI_UWB_MISO_GPIO_PIN,
		.dev = NULL,
	},
	.cs_pin = {
		.port = CONFIG_SPI_UWB_CS_GPIO_PORT,
		.pin_number = CONFIG_SPI_UWB_CS_GPIO_PIN,
		.dev = NULL,
	},
	.freq_hz = CONFIG_SPI_UWB_SLOW_RATE_FREQ, /* Start with slow rate freq. */
	.irq_priority = CONFIG_SPI_UWB_IRQ_PRIORITY,
	.op_flags = QSPI_OP_FLAGS,
};
