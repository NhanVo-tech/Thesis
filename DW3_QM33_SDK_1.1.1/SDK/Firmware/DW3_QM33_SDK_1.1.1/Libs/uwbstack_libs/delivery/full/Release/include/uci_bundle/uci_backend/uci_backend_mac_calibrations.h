/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <uci/uci.h>

struct uwbmac_context;

/**
 * DOC: Backend MAC calibrations overview
 *
 * The uci_backend_mac_calibrations allows to set configuration and calibration
 * values through UCI commands.
 *
 * Those commands uses the QORVO MCPS GID, the same as uci_backend_mac, it can
 * be used alone when other MAC commands through UCI are not needed.
 */

/*
 * Private data that should not be used by the client.
 * Holds backend internal state.
 */
struct uci_backend_mac_calibrations_context {
	struct uwbmac_context *uwbmac_context;
	struct uci_message_handlers uci_mac_handlers;
};

/**
 * uci_backend_mac_calibrations_init() - Attach this backend to the uci context
 * to bridge uci communication to the MAC.
 * @context: internal backend context, to be allocated by the client.
 * @uci: UCI context.
 * @uwbmac_context: opaque, implementation dependant uwbmac context.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr uci_backend_mac_calibrations_init(struct uci_backend_mac_calibrations_context *context,
					    struct uci *uci, struct uwbmac_context *uwbmac_context);
