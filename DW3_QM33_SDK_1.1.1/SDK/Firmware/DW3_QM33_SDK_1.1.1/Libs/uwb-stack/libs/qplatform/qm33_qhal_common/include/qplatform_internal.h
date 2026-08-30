/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include "qerr.h"
#include "qgpio.h"
#include "qspi.h"

/* By default, slow rate SPI freq for DW3000/QM33 is 4 Mhz. */
#ifndef CONFIG_SPI_UWB_SLOW_RATE_FREQ
#define CONFIG_SPI_UWB_SLOW_RATE_FREQ 4000000
#endif

/* By default, fast rate SPI freq for DW3000/QM33 is 32 Mhz. */
#ifndef CONFIG_SPI_UWB_FAST_RATE_FREQ
#define CONFIG_SPI_UWB_FAST_RATE_FREQ 32000000
#endif

/* QSPI flags: define QM33 as SPI master with specific config. */
#define QSPI_OP_FLAGS                               \
	(QSPI_MASTER | QSPI_MSB_FIRST | QSPI_CPOL | \
	 QSPI_SET_FRAME_LEN(CONFIG_UWB_SPI_FRAME_SIZE) | QSPI_MISO_SINGLE)

/* QM33 IRQ GPIO. */
extern const struct qgpio qm33_irq;

/* QM33 RSTn GPIO. */
extern const struct qgpio qm33_rstn;

/* QM33 QSPI. */
extern const struct qspi_instance qm33_qspi_instance;
extern struct qspi_config qm33_qspi_config;

/**
 * qplatform_uwb_read_irq_pin_value() -  Read IRQ pin value.
 *
 * Return: IRQ pin value.
 */
uint8_t qplatform_uwb_read_irq_pin_value(void);
