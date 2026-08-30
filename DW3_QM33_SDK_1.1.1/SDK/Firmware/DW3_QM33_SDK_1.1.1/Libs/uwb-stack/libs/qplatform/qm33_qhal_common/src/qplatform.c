/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qplatform.h"

#include "qassert.h"
#include "qplatform_internal.h"
#include "qpwr.h"
#include "qtime.h"

#include <string.h>

#define LOG_TAG "qplatform"
#include "deca_interface.h"
#include "qlog.h"

/* Delays */
#define RESET_LOW_DELAY_MS (1)
#define INITIAL_COLD_RESET_DELAY_MS (5)

/* SPI buffers */
#define DATA_BUFFER_SIZE 200
static uint8_t temp_tx_buf[DATA_BUFFER_SIZE];
static uint8_t temp_rx_buf[DATA_BUFFER_SIZE];

/* UWB IRQ Flags. */
#define QPLATFORM_UWB_IRQ_FLAGS (QGPIO_PULL_DOWN | QGPIO_INPUT | QGPIO_IRQ_EDGE_RISING)

/* DWT driver variables. */
static struct dwt_spi_s dwt_spi;
static struct dwt_probe_s dwt_probe_interf;
static struct dwchip_s dw;

/* Idle timer configuration. */
#ifdef CONFIG_QPLATFORM_IMPL_NRF
#define IDLE_TIMER_INSTANCE 2 /* RTC1 is used by freeRTOS. */
#define IDLE_TIMER_FREQUENCY 32768 /* 32768Hz freq. T = 30.5us. */
#define IDLE_TIMER_WIDTH QTIMER_WIDTH_24_BIT
#else
#error "Please provide idle timer configuration for selected platform."
#endif

static const struct qtimer_config idle_timer_config = { .freq_hz = IDLE_TIMER_FREQUENCY,
							.irq_priority = 7,
							.width = IDLE_TIMER_WIDTH };

#if !defined(CONFIG_DW3000) && !defined(CONFIG_DW3720)
#error "No Driver for selected UWB chip"
#endif

#ifdef CONFIG_DW3000
extern const struct dwt_driver_s dw3000_driver;
#endif
#ifdef CONFIG_DW3720
extern const struct dwt_driver_s dw3720_driver;
#endif

const struct dwt_driver_s *qplatform_dwt_drv_list[] = {
#ifdef CONFIG_DW3000
	&dw3000_driver,
#endif
#ifdef CONFIG_DW3720
	&dw3720_driver,
#endif
};

static struct qspi *qm33_uwb_spi;

static int32_t qplatform_uwb_spi_write_crc(uint16_t headerLength, const uint8_t *headerBuffer,
					   uint16_t bodyLength, const uint8_t *bodyBuffer,
					   uint8_t crc8)
{
	/* Function not implemented for now but it can't be NULL as it's always
	 * called by DWT driver. */
	return QERR_ENOTSUP;
}

static enum qerr qplatform_uwb_spi_transfer(uint16_t tx_len, uint8_t *tx_buf, uint16_t rx_len,
					    uint8_t *rx_buf)
{
	enum qerr r;
	const struct qspi_transfer txrx_desc = {
		.rx_buf = rx_buf,
		.rx_size = rx_len,
		.tx_buf = tx_buf,
		.tx_size = tx_len,
		.flags = 0,
	};

	r = qspi_transceive(qm33_uwb_spi, &txrx_desc);
	if (r)
		QLOGE("qspi_transceive failed (%d)", r);

	return r;
}

static int32_t qplatform_uwb_spi_read(uint16_t header_length, uint8_t *header_buffer,
				      uint16_t read_length, uint8_t *read_buffer)
{
	/* Read always bring header length on return. */
	uint32_t rxtx_size = read_length + header_length;
	uint8_t *temp_buf = temp_tx_buf;
	enum qerr r;

	if (rxtx_size > DATA_BUFFER_SIZE)
		return QERR_ENOMEM;

	memcpy(temp_buf, header_buffer, header_length);
	temp_buf += header_length;
	memset(temp_buf, 0x00, read_length);

	r = qplatform_uwb_spi_transfer(rxtx_size, temp_tx_buf, rxtx_size, temp_rx_buf);
	if (r)
		return r;

	temp_buf = temp_rx_buf + header_length;

	/* Remove received address character. */
	memcpy(read_buffer, temp_buf, read_length);

	return QERR_SUCCESS;
}

static int32_t qplatform_uwb_spi_write(uint16_t header_length, const uint8_t *header_buffer,
				       uint16_t body_length, const uint8_t *body_buffer)
{
	uint32_t rxtx_size = body_length + header_length;
	uint8_t *temp_buf = temp_tx_buf;
	enum qerr r;

	if (rxtx_size > DATA_BUFFER_SIZE)
		return QERR_ENOMEM;

	memcpy(temp_buf, header_buffer, header_length);
	temp_buf += header_length;
	memcpy(temp_buf, body_buffer, body_length);

	r = qplatform_uwb_spi_transfer(rxtx_size, temp_tx_buf, rxtx_size, temp_rx_buf);
	return r;
}

uint8_t qplatform_uwb_read_irq_pin_value(void)
{
	uint8_t en = 0;
	enum qerr r = qgpio_pin_read(&qm33_irq, &en);
	QASSERT(!r);
	return en;
}

static void qplatform_uwb_spi_irq_handler(void *arg)
{
	while (qplatform_uwb_read_irq_pin_value())
		dwt_isr();
}

enum qerr qplatform_uwb_interrupt_enable(void)
{
	enum qerr r;

	r = qgpio_pin_configure(&qm33_irq, QGPIO_INPUT);
	if (r)
		return r;

	r = qgpio_pin_irq_configure(&qm33_irq, QPLATFORM_UWB_IRQ_FLAGS);
	return r;
}

enum qerr qplatform_uwb_interrupt_disable(void)
{
	return qgpio_pin_irq_configure(&qm33_irq, QGPIO_IRQ_DISABLED);
}

void qplatform_uwb_spi_set_fast_rate_freq(void)
{
	qm33_qspi_config.freq_hz = CONFIG_SPI_UWB_FAST_RATE_FREQ;
	qspi_configure(qm33_uwb_spi, &qm33_qspi_config);

	/* Needed for fast rate. */
	qgpio_pin_configure(&qm33_qspi_config.sck_pin, QGPIO_OUTPUT_HIGH);
	qgpio_pin_configure(&qm33_qspi_config.mosi_pin, QGPIO_OUTPUT_HIGH);
}

void qplatform_uwb_spi_set_slow_rate_freq(void)
{
	qm33_qspi_config.freq_hz = CONFIG_SPI_UWB_SLOW_RATE_FREQ;
	qspi_configure(qm33_uwb_spi, &qm33_qspi_config);
}

static enum qerr qplatform_uwb_spi_init(struct dwt_spi_s *spi)
{
	enum qerr r;

	/* Generous initial init/wake-up-idle delay. */
	qtime_msleep(INITIAL_COLD_RESET_DELAY_MS);

	qm33_uwb_spi = qspi_open(&qm33_qspi_instance);
	if (!qm33_uwb_spi) {
		QLOGE("Failed to open QM33 qspi");
		return QERR_ENODEV;
	}

	r = qspi_configure(qm33_uwb_spi, &qm33_qspi_config);
	if (r) {
		QLOGE("Failed to configure QM33 qspi");
		return r;
	}

	/* At cold start, SPI communication rate cannot be higher than 7 Mhz. */
	qplatform_uwb_spi_set_slow_rate_freq();

	spi->readfromspi = qplatform_uwb_spi_read;
	spi->writetospi = qplatform_uwb_spi_write;
	spi->setfastrate = qplatform_uwb_spi_set_fast_rate_freq;
	spi->setslowrate = qplatform_uwb_spi_set_slow_rate_freq;
	spi->writetospiwithcrc = qplatform_uwb_spi_write_crc;

	/* Configure SPI IRQ handler. */
	r = qgpio_pin_irq_set_callback(&qm33_irq, qplatform_uwb_spi_irq_handler, NULL);
	if (r) {
		QLOGE("Failed to set QM33 SPI IRQ callback");
		return r;
	}

	QLOGD("%s suceeded\n", __func__);
	return QERR_SUCCESS;
}

static enum qerr qplatform_uwb_spi_deinit(void)
{
	return qspi_close(qm33_uwb_spi);
}

static void qplatform_uwb_wakeup(void)
{
	qpwr_uwb_wakeup();
}

enum qerr qplatform_init(void)
{
	enum qerr r;

	r = qplatform_uwb_interrupt_disable();
	if (r)
		return r;

	qplatform_uwb_reset();

	r = qplatform_uwb_spi_init(&dwt_spi);
	if (r)
		return r;

	dw.coex_gpio_pin = -1;
	dwt_probe_interf.dw = &dw;
	dwt_probe_interf.spi = &dwt_spi;
	dwt_probe_interf.wakeup_device_with_io = qplatform_uwb_wakeup;

	/* Provide the list of all the drivers supported.
	 * DWT driver will choose the correct one depending on the chip device ID. */
	dwt_probe_interf.driver_list = (struct dwt_driver_s **)qplatform_dwt_drv_list;
	dwt_probe_interf.dw_driver_num =
		(sizeof(qplatform_dwt_drv_list) / sizeof((qplatform_dwt_drv_list)[0]));

	return dwt_probe(&dwt_probe_interf);
}

enum qerr qplatform_deinit(void)
{
	enum qerr r = qplatform_uwb_interrupt_disable();
	if (r)
		return r;

	return qplatform_uwb_spi_deinit();
}

void qplatform_uwb_reset(void)
{
	qgpio_pin_configure(&qm33_rstn, QGPIO_OUTPUT_LOW);
	qtime_msleep(RESET_LOW_DELAY_MS);
	qgpio_pin_configure(&qm33_rstn, QGPIO_INPUT);
	qtime_msleep(RESET_LOW_DELAY_MS);
	qplatform_uwb_spi_set_slow_rate_freq();
}

#define DW3000_WAKEUP_LATENCY_US 2200
#define QM33_WAKEUP_LATENCY_US 3400

enum qerr qplatform_get_wakeup_latency(uint16_t *wakeup_latency_us)
{
	uint32_t dev_id = dwt_readdevid();

	switch (dev_id) {
	case DWT_DW3000_DEV_ID:
	case DWT_DW3000_PDOA_DEV_ID:
		*wakeup_latency_us = DW3000_WAKEUP_LATENCY_US;
		break;
	case DWT_QM33110_DEV_ID:
	case DWT_QM33120_PDOA_DEV_ID:
		*wakeup_latency_us = QM33_WAKEUP_LATENCY_US;
		break;
	default:
		return QERR_EIO;
	}

	return QERR_SUCCESS;
}

void qplatform_get_idle_timer_config(struct qtimer_config const **config)
{
	*config = &idle_timer_config;
}

uint8_t qplatform_get_idle_timer_instance(void)
{
	return IDLE_TIMER_INSTANCE;
}
