/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <qerr.h>
#include <stdint.h>

/**
 * DOC: Coordinator overview
 *
 * The uci_backend_coordinator implements MAC scheduler and region selection for
 * the protocol backends (Fira, PCTT, Radar). It also implements device state
 * management.
 *
 * The various backends are not aware of the state of scheduler or region when a
 * session of a certain type is required, or what state the device is in (active
 * or not). This interface provides a way for them to request a certain state.
 * The coordinator implementation is meant to be custom depending on the
 * project. It is meant to vary depending on the use case, target and so on. A
 * default implementation is provided.
 *
 */

struct uci_backend_coordinator;
struct uwbmac_context;

/**
 * struct uci_backend_coordinator_ops - Operations provided by the coordinator.
 */
struct uci_backend_coordinator_ops {
	/**
	 * @request_region: Callback invoked when a backend is requesting a
	 * certain region.
	 */
	enum qerr (*request_region)(struct uci_backend_coordinator *coord,
				    struct uwbmac_context *context, const char *region_name);
	/**
	 * @release_region: Callback invoked when a backend no longer needs a
	 * certain region.
	 */
	void (*release_region)(struct uci_backend_coordinator *coord,
			       struct uwbmac_context *context, const char *region_name);
	/**
	 * @get_scheduler: Get the scheduler used for the given region.
	 */
	const char *(*get_scheduler)(struct uci_backend_coordinator *coord,
				     const char *region_name);
	/**
	 * @get_region_id: Get the region id used for the given region.
	 *
	 * The region id will be store in parameter ``*region_id``.
	 *
	 * Return: QERR_SUCCESS on success or an error code.
	 */
	enum qerr (*get_region_id)(struct uci_backend_coordinator *coord, const char *region_name,
				   uint32_t *region_id);
};

/**
 * struct uci_backend_coordinator - UCI generic backend coordinator.
 *
 * Coordinator is a mechanism for backends to handle device state and region
 * state without knowing each others.
 */
struct uci_backend_coordinator {
	/**
	 * @ops: Coordinator operations.
	 */
	const struct uci_backend_coordinator_ops *ops;
};
