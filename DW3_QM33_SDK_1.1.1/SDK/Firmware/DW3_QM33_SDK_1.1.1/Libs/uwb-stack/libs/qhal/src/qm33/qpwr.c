/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <deca_device_api.h>
#include <qerr.h>
#include <qgpio.h>
#include <qirq.h>
#include <qpwr.h>
#include <qspi.h>
#include <qtime.h>

#if defined(__ZEPHYR__) && !((KERNEL_VERSION_MAJOR >= 3) && (KERNEL_VERSION_MINOR >= 1))
enum qerr qpwr_uwb_sleep(void)
{
	return QERR_ENOTSUP;
}

enum qerr qpwr_uwb_wakeup(void)
{
	return QERR_ENOTSUP;
}

bool qpwr_uwb_is_sleeping(void)
{
	return false;
}
#else
/*
 * The following gpios and spi configuration is a duplicate of
 * llhw/core/src/dw3000/qm33_uwb_driver_api.c
 * This is a temporary workaround until qhal becomes qplatform.
 */

/* Slow rate SPI freq. for DW3000/QM33 is 4 Mhz. */
#ifndef CONFIG_SPI_UWB_SLOW_RATE_FREQ
#define CONFIG_SPI_UWB_SLOW_RATE_FREQ 4000000
#endif

/* Time needed for CSn low to wake up chip. */
#define WAKEUP_CS_TOGGLE_US 400

/* Time needed for the chip to be awake, starting from the release of the CS. */
#define WAKEUP_DELAY_US 500

#define CONFIG_DWT_IRQ_FLAGS (QGPIO_PULL_DOWN | QGPIO_INPUT | QGPIO_IRQ_EDGE_RISING)

#define QSPI_OP_FLAGS                               \
	(QSPI_MASTER | QSPI_MSB_FIRST | QSPI_CPOL | \
	 QSPI_SET_FRAME_LEN(CONFIG_UWB_SPI_FRAME_SIZE) | QSPI_MISO_SINGLE)

/*
 * SPI pins (MOSI, MISO, SCK) are manaqed internally by Zephyr, no need to set a
 * device address.
 */
#define CONFIG_SPI_UWB_SCK_GPIO_DEV NULL
#define CONFIG_SPI_UWB_MOSI_GPIO_DEV NULL
#define CONFIG_SPI_UWB_MISO_GPIO_DEV NULL

#ifdef CONFIG_QHAL_IMPL_ZEPHYR
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>

#if !DT_NODE_HAS_STATUS(DT_NODELABEL(dw35720), okay)
#error "DW3572 SPI device must be declared and enabled on device tree definitiions with dw35720 as node label"
#endif

/* Retrieve SPI information via device tree. */
static const struct spi_config qm33_spi_config =
	SPI_CONFIG_DT(DT_NODELABEL(dw35720), QSPI_OP_FLAGS, 0);

#define CONFIG_SPI_UWB_SCK_GPIO_PORT 0
#define CONFIG_SPI_UWB_SCK_GPIO_PIN 0

#define CONFIG_SPI_UWB_MOSI_GPIO_PORT 0
#define CONFIG_SPI_UWB_MOSI_GPIO_PIN 0

#define CONFIG_SPI_UWB_MISO_GPIO_PORT 0
#define CONFIG_SPI_UWB_MISO_GPIO_PIN 0

#define CONFIG_SPI_UWB_CS_GPIO_PORT 0
#define CONFIG_SPI_UWB_CS_GPIO_PIN 0

#define CONFIG_SPI_UWB_CS_GPIO_DEV (void *)&qm33_spi_config.cs->gpio

#else
/* This defines will be generated via Kconfig on UWB-10774 with no need to
 * identify OS. */

#define CONFIG_SPI_UWB_CS_GPIO_DEV NULL

#endif

static struct qspi_config qm33_qspi_config = {
	.sck_pin = {
		.port = CONFIG_SPI_UWB_SCK_GPIO_PORT,
		.pin_number = CONFIG_SPI_UWB_SCK_GPIO_PIN,
		.dev = CONFIG_SPI_UWB_SCK_GPIO_DEV,
	},
	.mosi_pin = {
		.port = CONFIG_SPI_UWB_MOSI_GPIO_PORT,
		.pin_number = CONFIG_SPI_UWB_MOSI_GPIO_PIN,
		.dev = CONFIG_SPI_UWB_MOSI_GPIO_DEV,
	},
	.miso_pin = {
		.port = CONFIG_SPI_UWB_MISO_GPIO_PORT,
		.pin_number = CONFIG_SPI_UWB_MISO_GPIO_PIN,
		.dev = CONFIG_SPI_UWB_MISO_GPIO_DEV,
	},
	.cs_pin = {
		.port = CONFIG_SPI_UWB_CS_GPIO_PORT,
		.pin_number = CONFIG_SPI_UWB_CS_GPIO_PIN,
		.dev = CONFIG_SPI_UWB_CS_GPIO_DEV,
	},
	.freq_hz = CONFIG_SPI_UWB_SLOW_RATE_FREQ, /* Slow initial rate. */

	.irq_priority = CONFIG_SPI_UWB_IRQ_PRIORITY,

	.op_flags = QSPI_OP_FLAGS,
};

/**
 * QM33 is sleeping when powered on, wakeup callback and qpwr_uwb_wakeup() are called during the
 * probing. See dwt_probe() in deca_compat.c.
 */
static bool is_sleeping = true;

enum qerr qpwr_uwb_sleep(void)
{
	int key = qirq_lock();
	if (!is_sleeping) {
		dwt_entersleep(DWT_DW_IDLE_RC);
		is_sleeping = true;
	}
	qirq_unlock(key);
	return QERR_SUCCESS;
}

enum qerr qpwr_uwb_wakeup(void)
{
	int key = qirq_lock();
	if (is_sleeping) {
		qgpio_pin_configure(&qm33_qspi_config.cs_pin, QGPIO_OUTPUT_LOW);
		qtime_usleep(WAKEUP_CS_TOGGLE_US);
		qgpio_pin_configure(&qm33_qspi_config.cs_pin, QGPIO_OUTPUT_HIGH);
		qtime_usleep(WAKEUP_DELAY_US);
		is_sleeping = false;
	}
	qirq_unlock(key);
	return QERR_SUCCESS;
}

bool qpwr_uwb_is_sleeping(void)
{
	return is_sleeping;
}
#endif
