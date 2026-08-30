/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <uci/uci.h>
#include <uci_backend/uci_antenna_helper.h>
#include <uci_backend/uci_backend_coordinator.h>
#include <uci_backend/uci_backend_core.h>
#include <uci_backend/uci_backend_manager.h>
#include <uwbmac/fira_helper.h>

struct uwbmac_context;

/**
 * DOC: Backend FiRa overview
 *
 * The uci_backend_fira implements the control of ranging type session.
 *
 * For every UCI commands relative to a session, the backend manager will
 * dispatch it to this fira backend if the session is of type ranging. Then this
 * fira backend will realize the command through uwbmac API using the fira
 * helper interface. This will then be realized by the fira region inside the
 * MAC.
 *
 */

/**
 * struct uci_backend_fira_context - Definition of this backend context.
 */
struct uci_backend_fira_context {
	/**
	 * @fira_context: context for fira helper.
	 */
	struct fira_context fira_context;
	/**
	 * @uwbmac_context: UWB MAC context.
	 */
	struct uwbmac_context *uwbmac_context;
	/**
	 * @antennas: antennas parameters.
	 */
	struct antenna_parameters *antennas;
	/**
	 * @uci: uci context used in callbacks from the helper.
	 */
	struct uci *uci;
	/**
	 * @uci_backend_core_context: UCI core backend context.
	 */
	struct uci_backend_core_context *uci_backend_core_context;
	/**
	 * @coord: uci backend coordinator.
	 */
	struct uci_backend_coordinator *coord;
	/**
	 * @sess_man: the root backend manager.
	 */
	struct uci_backend_manager *sess_man;
	/**
	 * @session_ops: Ops for the FiRa backend.
	 */
	struct uci_session_controller_ops session_ops;
};

/**
 * uci_backend_fira_init() - Attach this backend to the uci context to bridge
 * uci communication to the MAC.
 * @context: FIRA context.
 * @uci_backend_core_context: Core UCI backend context.
 * @uci: UCI context.
 * @uwbmac_context: UWB MAC context.
 * @coord: backend coordinator.
 * @sess_man: session manager.
 *
 * Return: QERR_SUCCESS or an error code.
 */
enum qerr uci_backend_fira_init(struct uci_backend_fira_context *context,
				struct uci_backend_core_context *uci_backend_core_context,
				struct uci *uci, struct uwbmac_context *uwbmac_context,
				struct uci_backend_coordinator *coord,
				struct uci_backend_manager *sess_man);

/**
 * uci_backend_fira_set_antenna_conf() - Give backend the antenna conf to
 * configure session according to aoa requests.
 * @context: This backend context.
 * @antennas_params: The antenna configuration.
 */
void uci_backend_fira_set_antenna_conf(struct uci_backend_fira_context *context,
				       struct antenna_parameters *antennas_params);

/**
 * uci_backend_fira_get_supported_channels() - Get supported channels.
 * @context: This backend context.
 * @channel_number: Supported channels.
 *
 * Return: QERR_SUCCESS or an error code.
 */
enum qerr uci_backend_fira_get_supported_channels(struct uci_backend_fira_context *context,
						  uint16_t *channel_number);

/**
 * uci_backend_fira_release() - Free global resources used.
 * @context: This backend context.
 */
void uci_backend_fira_release(struct uci_backend_fira_context *context);
