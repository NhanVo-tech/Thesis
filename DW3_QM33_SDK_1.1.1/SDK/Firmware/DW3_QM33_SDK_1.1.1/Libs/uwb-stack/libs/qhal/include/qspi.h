/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qerr.h>
#include <qgpio.h>
#include <stdbool.h>
#include <stdint.h>

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

/* SPI Descriptor (opaque). */
struct qspi;

/**
 * struct qspi_instance - spi input parameters.
 * @instance_number: SPI instance number. Implementation dependent.
 * @dev: Device pointer. Implementation dependent.
 */
struct qspi_instance {
	uint32_t instance_number;
	const void *dev;
};

/* QSPI operational modes. */

/**
 * QSPI_MASTER - Master mode. Used in &qspi_config.op_flags field.
 */
#define QSPI_MASTER BIT_TO_0(0)

/**
 * QSPI_SLAVE - Slave mode. Used in &qspi_config.op_flags field.
 */
#define QSPI_SLAVE BIT(0)

/**
 * QSPI_ROLE_MASK - Master/Slave mode mask.
 */
#define QSPI_ROLE_MASK 0x01U

/**
 * QSPI_GET_ROLE - Get Master/Slave mode.
 * @flags: QSPI bitfield flags from &qspi_config.op_flags.
 * Return: Master/Slave mode
 */
#define QSPI_GET_ROLE(flags) ((flags)&QSPI_ROLE_MASK)

/* */

/**
 * QSPI_CPOL - QSPI clock polarity, 0 to LOW, 1 to HIGH, used in combination with clock
 * phase. Used in &qspi_config.op_flags field. See &QSPI_CPHA
 *
 * * %if QSPI_MODE_CPOL=0 and QSPI_CPHA=0, sample on leading edge.
 * * %if QSPI_MODE_CPOL=0 and QSPI_CPHA=1, sample on trailing edge.
 * * %if QSPI_MODE_CPOL=1 and QSPI_CPHA=0, sample on leading edge.
 * * %if QSPI_MODE_CPOL=1 and QSPI_CPHA=1, sample on trailing edge.
 */
#define QSPI_CPOL BIT(1)

/**
 * QSPI_CPHA - QSPI clock phase, used in combination with clock polarity.
 * Used in &qspi_config.op_flags field.
 * See &QSPI_CPOL
 *
 * * %if QSPI_MODE_CPOL=0 and QSPI_CPHA=0, sample on leading edge.
 * * %if QSPI_MODE_CPOL=0 and QSPI_CPHA=1, sample on trailing edge.
 * * %if QSPI_MODE_CPOL=1 and QSPI_CPHA=0, sample on leading edge.
 * * %if QSPI_MODE_CPOL=1 and QSPI_CPHA=1, sample on trailing edge.
 *
 */
#define QSPI_CPHA BIT(2)

/**
 * QSPI_LOOP - Loopback mode.
 * Used in &qspi_config.op_flags field.
 *
 * * %1 - loopback mode enabled.
 * * %0 - loopback mode disabled.
 */
#define QSPI_LOOP BIT(3)

/* Frame Bit order. */
/**
 * QSPI_MSB_FIRST - MSB first for transfers. Used in &qspi_config.op_flags field.
 */
#define QSPI_MSB_FIRST BIT_TO_0(4)

/**
 * QSPI_LSB_FIRST - LSB first for transfers. Used in &qspi_config.op_flags.
 */
#define QSPI_LSB_FIRST BIT(4)

/* Frame size. */
/**
 * QSPI_FRAME_SIZE_SHIFT - Frame size shift in &qspi_config.op_flags.
 */
#define QSPI_FRAME_SIZE_SHIFT (5U)

/**
 * QSPI_FRAME_SIZE_MASK - Frame size mask.
 */
#define QSPI_FRAME_SIZE_MASK (0x3FU << QSPI_FRAME_SIZE_SHIFT)

/**
 * QSPI_FRAMED_SIZE_GET - Get frame size from &qspi_config.op_flags.
 * @flags: QSPI bitfield flags from &qspi_config.op_flags.
 * Return: Frame length in bytes.
 */
#define QSPI_FRAMED_SIZE_GET(flags) (((flags)&SPI_FRAME_SIZE_MASK) >> QSPI_FRAME_SIZE_SHIFT)
/**
 * QSPI_SET_FRAME_LEN() - Set frame size on QSPI_FRAME_SIZE_SHIFT bits for a maximum of 64 bytes,
 * inside &qspi_config.op_flags.
 * @frame_length: Frame length in bytes.
 */
#define QSPI_SET_FRAME_LEN(frame_length) ((frame_length) << QSPI_FRAME_SIZE_SHIFT)

/**
 * QSPI_CS_ACTIVE_HIGH - Chip/Slave select pin logic.
 * 1 is CS pin is active HIGH, 0 if active LOW.
 * Used in &qspi_config.op_flags field.
 */
#define QSPI_CS_ACTIVE_HIGH BIT(14)

/**
 * QSPI_MISO_SINGLE - Extended mode for MISO single line.
 * Used in &qspi_config.op_flags.
 */
#define QSPI_MISO_SINGLE (0U << 16)
/**
 * QSPI_MISO_DUAL - Extended mode for MISO dual line.
 * Used in &qspi_config.op_flags.
 */
#define QSPI_MISO_DUAL (1U << 16)
/**
 * QSPI_MISO_QUAD - Extended mode for MISO quad line.
 * Used in &qspi_config.op_flags.
 */
#define QSPI_MISO_QUAD (2U << 16)
/**
 * QSPI_MISO_OCTAL - Extended mode for MISO octal line.
 * Used in &qspi_config.op_flags.
 */
#define QSPI_MISO_OCTAL (3U << 16)

/**
 * QSPI_LINES_MASK - Extended mode mask.
 */
#define QSPI_LINES_MASK (0x3U << 16)

/**
 * QSPI_HALF_DUPLEX - QSPI half duplex transaction mode.
 * Used in &qspi_config.op_flags.
 */
#define QSPI_HALF_DUPLEX BIT_TO_0(18)

/**
 * QSPI_FULL_DUPLEX - QSPI full duplex transaction mode.
 * Used in &qspi_config.op_flags.
 */
#define QSPI_FULL_DUPLEX BIT(18)

/**
 * QSPI_DUPLEX_MASK - QSPI transaction mode mask.
 */
#define QSPI_DUPLEX_MASK (0x1U << 18)

/**
 * struct qspi_config - QSPI configuration.
 *
 * @sck_pin: Serial clock pin descriptor.
 * @cs_pin: Chip select pin descriptor.
 * @mosi_pin: MOSI pin descriptor.
 * @miso_pin: MISO pin descriptor.
 * @freq_hz: SPI frequency in Hz.
 * @irq_priority: SPI IRQ priority.
 * @op_flags: Bit field operation flags.
 */
struct qspi_config {
	struct qgpio sck_pin;
	struct qgpio cs_pin;
	struct qgpio mosi_pin;
	struct qgpio miso_pin;
	uint32_t freq_hz;
	uint8_t irq_priority;
	uint32_t op_flags;
};

/**
 * struct qspi_transfer - QSPI transfer data.
 *
 * @tx_buf: Pointer to transmit buffer.
 * @tx_size: Size of transmit buffer.
 * @rx_buf: Pointer to receive buffer.
 * @rx_size: Size of receive buffer.
 * @flags: Specific transfer flags. Implementation dependent.
 */
struct qspi_transfer {
	uint8_t *tx_buf;
	uint8_t *rx_buf;
	uint32_t tx_size;
	uint32_t rx_size;
	uint32_t flags;
};

/**
 * typedef qspi_xfer_cb - Pointer to callback function.
 * @arg: private data returned after transfer.
 *
 * Return: nothing.
 */
typedef void (*qspi_xfer_cb)(void *arg);

/**
 * qspi_open() - Create a SPI descriptor for a given SPI instance.
 *
 * NOTE: This function should be called first.
 *
 * @instance: SPI instance to open. How to fill it is implementation dependent.
 *
 * Return: Pointer to the initialized SPI descriptor.
 */
struct qspi *qspi_open(const struct qspi_instance *instance);

/**
 * qspi_close() - Destroy a SPI descriptor.
 * @spi: Pointer to the SPI descriptor.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qspi_close(struct qspi *spi);

/**
 * qspi_configure() - Initialize and configure an SPI device.
 *
 * NOTE: This function should be called before any transfers.
 *
 * @spi: Pointer to the SPI descriptor.
 * @config: Pointer to SPI configuration.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qspi_configure(struct qspi *spi, const struct qspi_config *config);

/**
 * qspi_irq_set_callback() - Set callback function to be called when SPI IRQ.
 * @spi: Pointer to the SPI descriptor.
 * @handler: Callback to call when xfer done in async mode.
 *           If NULL, synchronous mode is used.
 * @arg: Private data of the xfer done callback.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qspi_irq_set_callback(struct qspi *spi, qspi_xfer_cb handler, void *arg);

/**
 * qspi_transceive() - Make a SPI transaction.
 * @spi: Pointer to the SPI descriptor.
 * @xfer: Pointer to the SPI transfer data.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr qspi_transceive(struct qspi *spi, const struct qspi_transfer *xfer);
