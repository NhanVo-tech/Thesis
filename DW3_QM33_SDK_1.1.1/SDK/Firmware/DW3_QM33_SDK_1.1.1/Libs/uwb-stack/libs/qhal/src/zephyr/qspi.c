/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <qerr.h>
#include <qspi.h>
#include <stddef.h>

struct qspi *qspi_open(const struct qspi_instance *instance)
{
	return NULL;
}

enum qerr qspi_close(struct qspi *spi)
{
	return QERR_ENOTSUP;
}

enum qerr qspi_configure(struct qspi *spi, const struct qspi_config *config)
{
	return QERR_ENOTSUP;
}

enum qerr qspi_irq_set_callback(struct qspi *spi, qspi_xfer_cb handler, void *arg)
{
	return QERR_ENOTSUP;
}

enum qerr qspi_transceive(struct qspi *spi, const struct qspi_transfer *xfer)
{
	return QERR_ENOTSUP;
}
