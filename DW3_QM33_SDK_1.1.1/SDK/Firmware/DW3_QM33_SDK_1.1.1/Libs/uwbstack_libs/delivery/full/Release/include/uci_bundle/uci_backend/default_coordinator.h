/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <uci_backend/uci_backend_coordinator.h>

struct uwbmac_context;

/**
 * enum uwbmac_scheduler - Internal use.
 *
 * @UWBMAC_SCHEDULER_ENDLESS: Endless scheduler.
 * @UWBMAC_SCHEDULER_ON_DEMAND: On demand scheduler.
 * @UWBMAC_SCHEDULER_NONE: No scheduler.
 */
enum uwbmac_scheduler {
	UWBMAC_SCHEDULER_ENDLESS,
	UWBMAC_SCHEDULER_ON_DEMAND,
	UWBMAC_SCHEDULER_NONE,
};

/**
 * enum uwbmac_region - Internal use.
 *
 * @UWBMAC_REGION_FBS: FBS region.
 * @UWBMAC_REGION_PCTT: PCTT region.
 * @UWBMAC_REGION_NONE: No region.
 */
enum uwbmac_region {
	UWBMAC_REGION_FBS,
	UWBMAC_REGION_PCTT,
	UWBMAC_REGION_NONE,
};

/**
 * struct default_coordinator - A coordinator implementation.
 *
 * NOTE: This implementation only works with one device.
 *
 * @start_count: Start counter.
 * @current_region: Current region.
 * @current_scheduler: Current scheduler.
 * @base: UCI generic backend coordinator.
 */
struct default_coordinator {
	/* private */
	enum uwbmac_scheduler current_scheduler;
	struct uci_backend_coordinator base;
};

/**
 * default_coordinator_init - Initialise default coordinator.
 * @coord: The default coordinator.
 * @context: The UWB MAC context.
 *
 * Return: QERR_SUCCESS on success or an error code.
 */
enum qerr default_coordinator_init(struct default_coordinator *coord,
				   struct uwbmac_context *context);

/**
 * default_request_region - Implementation of request_region interface.
 * @coord: The default coordinator.
 * @context: The UWB MAC context.
 * @region_name: The region being requested
 *
 * NOTE: This implementation only works when device is down.
 *
 * Return: QERR_SUCCESS on success or an error code.
 */
enum qerr default_request_region(struct uci_backend_coordinator *coord,
				 struct uwbmac_context *context, const char *region_name);

/**
 * default_release_region - Implementation of release_region interface.
 * @coord: The default coordinator.
 * @context: The UWB MAC context.
 * @region_name: The region that is not requested anymore.
 *
 * NOTE: This implementation only does something when device is down.
 * It will set default scheduler to reset it completely.
 *
 */
void default_release_region(struct uci_backend_coordinator *coord, struct uwbmac_context *context,
			    const char *region_name);

/**
 * default_get_scheduler - Implementation of get_scheduler interface.
 * @coord: The default coordinator.
 * @region_name: The region for which we want to know the scheduler.
 *
 * Return: cstring representing the chosen scheduler.
 */
const char *default_get_scheduler(struct uci_backend_coordinator *coord, const char *region_name);

/**
 * default_get_region_id - Implementation of get_region_id interface.
 * @coord: The default coordinator.
 * @region_name: The region for which we want to know the scheduler.
 * @region_id: Out param to store the region id.
 *
 * Return: QERR_SUCCESS on success or an error code.
 */
enum qerr default_get_region_id(struct uci_backend_coordinator *coord, const char *region_name,
				uint32_t *region_id);
