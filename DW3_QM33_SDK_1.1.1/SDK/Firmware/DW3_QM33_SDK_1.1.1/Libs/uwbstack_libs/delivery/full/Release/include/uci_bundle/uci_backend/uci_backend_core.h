/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <uci/uci.h>

/**
 * DOC: UCI Core backend overview
 *
 * This backend handles all UCI commands for the device itself, so-called "core"
 * commands. It is independent from all regions and will always be present where
 * the UCI protocol is supported.
 */

struct uci_message_builder;
struct uwbmac_context;

/**
 * struct uci_backend_core_ops - Backend operations for core commands.
 *
 * This backend is in charge of receiving the core UCI commands. It may need
 * assistance from specific backends when enabled for which it provides this
 * operations mechanism.
 *
 * Backends must register their operations using uci_backend_core_register_ops()
 * to be called back accordingly.
 */
struct uci_backend_core_ops {
	/**
	 * @device_reset: Handle device reset operations.
	 */
	void (*device_reset)(uint8_t reason, void *user_data);
	/**
	 * @get_caps_info: Get capability info for the region and add data to
	 * the given builder.
	 */
	enum qerr (*get_caps_info)(struct uci_message_builder *builder, int *number,
				   void *user_data);
	/**
	 * @get_device_info: Get device information.
	 */
	void (*get_device_info)(uint8_t *mac_major, uint8_t *mac_minor, uint8_t *phy_major,
				uint8_t *phy_minor, void *user_data);
	/**
	 * @ops_list_next: Next element in the list of backend-specific
	 * operations.
	 */
	struct uci_backend_core_ops *ops_list_next;
	/**
	 * @user_data: User data to be given when the op is called.
	 */
	void *user_data;
};

/**
 * struct device_params - Definition of device specific parameters.
 */
struct device_params {
	/**
	 * @pan_coord: The device is a PAN coordinator if true.
	 */
	bool pan_coord;
	/**
	 * @promiscuous_mode: The device current promiscuous mode.
	 */
	bool promiscuous_mode;
	/**
	 * @pan_id: The device current PAN ID.
	 */
	uint16_t pan_id;
	/**
	 * @short_addr: The device current short address.
	 */
	uint16_t short_addr;
	/**
	 * @extended_addr: The device current extended address.
	 */
	uint64_t extended_addr;
	/**
	 * @frame_retries: The device current frame retry.
	 */
	int frame_retries;
	/**
	 * @traces: Traces bit-field.
	 *
	 * Each bit enable (1) or disable (0) a specific trace module.
	 */
	uint32_t traces;
};

struct uci_backend_core_context {
	/**
	 * @uci: uci context used in callbacks from the helper.
	 */
	struct uci *uci;
	/**
	 * @uwbmac_context: UWB MAC context.
	 */
	struct uwbmac_context *uwbmac_context;
	/**
	 * @ops_list_head: Head of the list of backend-specific operations.
	 */
	struct uci_backend_core_ops *ops_list_head;
	/**
	 * @device_param: devices parameters.
	 */
	struct device_params device_param;
	/**
	 * @device_info_vendor_data: vendor data to include in core device info.
	 */
	const uint8_t *device_info_vendor_data;
	/**
	 * @device_info_vendor_length: length of vendor_data.
	 */
	size_t device_info_vendor_length;
};

/**
 * uci_backend_core_init() - Attach this backend to the uci context to bridge
 * uci communication to the MAC.
 * @context: FIRA context.
 * @uci: UCI context.
 * @uwbmac_context: UWB MAC context.
 *
 * Return: QERR_SUCCESS or an error code.
 */
enum qerr uci_backend_core_init(struct uci_backend_core_context *context, struct uci *uci,
				struct uwbmac_context *uwbmac_context);

/**
 * uci_backend_core_release() - Free global resources used.
 * @context: This backend context.
 */
void uci_backend_core_release(struct uci_backend_core_context *context);

/**
 * uci_backend_core_register_ops() - Register backend-specific operations.
 * @context: This backend context.
 * @ops: The registering backend operations.
 * @user_data: The registering backend context.
 *
 * Region-specific backends need to call this to register their operations,
 * which will be called back in turn when the corresponding command is received.
 *
 * Note that, if no callback is registered for the device_reset command, this
 * backend shall respond with a success message. Otherwise, the code handling
 * the reset must call uci_backend_core_reset_done() with the correst status
 * when done.
 */
void uci_backend_core_register_ops(struct uci_backend_core_context *context,
				   struct uci_backend_core_ops *ops, void *user_data);

/**
 * uci_backend_core_set_vendor_data() - Set for core device info vendor data.
 * @context: This backend context.
 * @data: Pointer to vendor data to include in core device info response.
 * @length: The vendor data length.
 */
void uci_backend_core_set_vendor_data(struct uci_backend_core_context *context, const uint8_t *data,
				      size_t length);

/**
 * uci_backend_core_reset_done() - Send response to core device reset.
 * @context: This backend context.
 * @success: Status of the reset command.
 *
 * Must be called once done resetting the chip, to send the UCI response back.
 * Also change the device state if success is true
 */
void uci_backend_core_reset_done(struct uci_backend_core_context *context, bool success);

/**
 * uci_backend_core_build_power_stats() - Build power statistics message.
 * @context: This backend context.
 * @builder: The builder for the message.
 *
 * Return: UCI status code.
 */
enum uci_status_code uci_backend_core_build_power_stats(struct uci_backend_core_context *context,
							struct uci_message_builder *builder);
