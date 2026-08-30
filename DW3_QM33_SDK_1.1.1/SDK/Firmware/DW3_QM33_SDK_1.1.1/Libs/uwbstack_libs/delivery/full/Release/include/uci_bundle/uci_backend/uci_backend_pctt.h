/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "uci_backend_fira.h"

#include <uci/uci.h>
#include <uci_backend/uci_backend_coordinator.h>
#include <uci_backend/uci_backend_manager.h>
#include <uwbmac/pctt_helper.h>

/**
 * DOC: Backend PCTT overview
 *
 * The uci_backend_pctt implements the control of test session.
 *
 * For every UCI commands relative to a test session, the backend manager will
 * dispatch it to this pctt backend if the session is of type test. Then this
 * pctt backend will realize the command through uwbmac API using the pctt
 * helper interface. This will then be realized by the pctt region inside the
 * MAC.
 *
 */

/**
 * struct uci_backend_pctt_context - Definition of this backend context.
 *
 * NOTE: UCI spec defines several command to set the configuration for a test.
 * It is consumed in one helper call, hence the context hold the various part of
 * the configuration.
 */
struct uci_backend_pctt_context {
	/**
	 * @pctt_context: context for pctt helper.
	 */
	struct pctt_context pctt_context;
	/**
	 * @uwbmac_context: UWB MAC context.
	 */
	struct uwbmac_context *uwbmac_context;
	/**
	 * @pctt_params: part of the test configuration held.
	 */
	struct pctt_parameters pctt_params;
	/**
	 * @pctt_session_params: part of the test configuration held.
	 */
	struct pctt_session_parameters pctt_session_params;
	/**
	 * @session_state: the unique session state as per uci spec.
	 */
	enum quwbs_fbs_session_state session_state;
	/**
	 * @uci: uci context used in callbacks from the helper.
	 */
	struct uci *uci;
	/**
	 * @coord: uci backend coordinator.
	 */
	struct uci_backend_coordinator *coord;
	/**
	 * @sess_man: common FiRa sub-modules sessions manager.
	 */
	struct uci_backend_manager *sess_man;
	/**
	 * @enable_diag: whether diagnostic notification is used.
	 */
	bool enable_diag;
	/**
	 * @session_ops: Ops for the PCTT backend.
	 */
	struct uci_session_controller_ops session_ops;
	/**
	 * @mandatory_control: mandatory parameters set control.
	 */
	uint32_t mandatory_control;
};

/**
 * uci_backend_pctt_init() - Attach this backend to the uci context to bridge
 * uci communication to the MAC.
 * @context: This backend context.
 * @uci: UCI context.
 * @uwbmac_context: UWB MAC context.
 * @coord: backend coordinator.
 * @sess_man: session manager.
 *
 * NOTE: This uci_backend_pctt is meant to be used in addition of
 * uci_backend_fira. It cannot be used alone, most of common FiRa errors are
 * dealt with in uci_backend_fira.
 *
 * Return: QERR_SUCCESS or error.
 */
enum qerr uci_backend_pctt_init(struct uci_backend_pctt_context *context, struct uci *uci,
				struct uwbmac_context *uwbmac_context,
				struct uci_backend_coordinator *coord,
				struct uci_backend_manager *sess_man);

/**
 * uci_backend_pctt_release() - Free global resources used.
 * @context: This backend context.
 */
void uci_backend_pctt_release(struct uci_backend_pctt_context *context);
