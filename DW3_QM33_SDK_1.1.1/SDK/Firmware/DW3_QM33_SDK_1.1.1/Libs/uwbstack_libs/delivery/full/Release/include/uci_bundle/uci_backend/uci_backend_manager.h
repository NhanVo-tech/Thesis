/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <net/fbs_region_params.h>
#include <net/fira_region_params.h>
#include <uci/uci.h>
#include <uci/uci_message.h>
#include <uci/uci_spec_fira.h>
#include <uci/uci_spec_qorvo.h>

#define MAX_NUMBER_OF_STYPES 2

struct uwbmac_context;

/**
 * DOC: Manager overview
 *
 * The uci_backend_manager implements command dispatching for the submodules it
 * manages. Various sub backends exists to handle FIRA, PCTT and Android CCC
 * ranging commands in a modular way. Thus, the manager will parse an UCI
 * command and select the correct sub-backend to call based on session type.
 *
 * The manager will become obsolete once all fbs region will be only accessed
 * through the fbs region.
 */

static const int UCI_SESSION_TYPE_ALL = -1;

struct uci_session;
struct uci_session_controller;

/**
 * struct uci_backend_manager - Definition of this backend context.
 */
struct uci_backend_manager {
	/**
	 * @head: Linked list of sessions.
	 */
	struct uci_session *head;
	/**
	 * @uwbmac_context: UWB MAC context.
	 */
	struct uwbmac_context *uwbmac_context;
	/**
	 * @uci: uci context used in callbacks from the helper.
	 */
	struct uci *uci;
	/**
	 * @controller_head: Head of the list of uci session operations.
	 */
	struct uci_session_controller *controller_head;
	/**
	 * @default_controller: Controller used if the session type is not
	 * handled by anyone.
	 */
	struct uci_session_controller *default_controller;
	/**
	 * @uwb_disabled: Wether uwb is disabled or not
	 */
	bool uwb_disabled;
};

/**
 * uci_backend_manager_init() - Initialize the manager and attach this backend
 * to the uci context.
 * @context: This backend context.
 * @uci: UCI context.
 * @uwbmac: UWB MAC context.
 */
void uci_backend_manager_init(struct uci_backend_manager *context, struct uci *uci,
			      struct uwbmac_context *uwbmac);

/**
 * uci_backend_send_session_ntf() - Sends a SESSION_STATUS_NTF.
 * @uci: UCI transport to use.
 * @session_handle: The session handle.
 * @state: The new state of the session.
 * @code: The reason of the state change.
 */
void uci_backend_send_session_ntf(struct uci *uci, uint32_t session_handle,
				  enum quwbs_fbs_session_state state,
				  enum quwbs_fbs_reason_code code);

/**
 * uci_backend_manager_release() - Free resources allocated.
 * @context: This backend context.
 */
void uci_backend_manager_release(struct uci_backend_manager *context);

/**
 * struct uci_session_controller_ops - The operations on session that are
 * dispatched.
 */
struct uci_session_controller_ops {
	/**
	 * @init: Initialize a session.
	 * @uci: uci context needed to send uci messages.
	 * @mt_gid_oid: Union of message type, group identifier and opcode
	 * identifier.
	 * @session_id: The session identifier that shall be initialized.
	 * @user_data: user data previously passed in
	 * uci_backend_manager_register.
	 *
	 * This is a mandatory Ops, if NULL uci_backend_manager_register will
	 * fail.
	 *
	 * Return: QERR_SUCCESS or an error.
	 */
	enum qerr (*init)(struct uci *uci, uint16_t mt_gid_oid, uint32_t session_id,
			  enum quwbs_fbs_session_type session_type, void *user_data);
	/**
	 * @deinit: Deinitialize a session.
	 * @uci: uci context needed to send uci messages.
	 * @mt_gid_oid: Union of message type, group identifier and opcode
	 * identifier.
	 * @session_handle: The session handle that shall be deinitialized.
	 * @user_data: user data previously passed in
	 * uci_backend_manager_register.
	 *
	 * This is a mandatory Ops, if NULL uci_backend_manager_register will
	 * fail.
	 *
	 * Return: QERR_SUCCESS or an error.
	 */
	enum qerr (*deinit)(struct uci *uci, uint16_t mt_gid_oid, uint32_t session_handle,
			    void *user_data);

	/**
	 * @start: Start given session.
	 * @uci: uci context needed to send uci messages.
	 * @mt_gid_oid: Union of message type, group identifier and opcode
	 * identifier.
	 * @session_handle: The handle for the session that should start.
	 * @user_data: user data previously passed in
	 * uci_backend_manager_register.
	 *
	 * Return: QERR_SUCCESS or an error.
	 */
	enum qerr (*start)(struct uci *uci, uint16_t mt_gid_oid, uint32_t session_handle,
			   void *user_data);

	/**
	 * @stop: Stop given session.
	 * @uci: uci context needed to send uci messages.
	 * @mt_gid_oid: Union of message type, group identifier and opcode
	 * identifier.
	 * @session_handle: The handle for the session that should stop.
	 * @user_data: user data previously passed in
	 * uci_backend_manager_register.
	 *
	 * Return: QERR_SUCCESS or an error.
	 */
	enum qerr (*stop)(struct uci *uci, uint16_t mt_gid_oid, uint32_t session_handle,
			  void *user_data);

	/**
	 * @set_app_config: Configure a session.
	 * @uci: uci context needed to send uci messages.
	 * @mt_gid_oid: Union of message type, group identifier and opcode
	 * identifier.
	 * @payload: The whole uci message to process.
	 * @user_data: user data previously passed in
	 * uci_backend_manager_register.
	 *
	 * This is a mandatory Ops, if NULL uci_backend_manager_register will
	 * fail.
	 *
	 * Return: QERR_SUCCESS or an error.
	 */
	enum qerr (*set_app_config)(struct uci *uci, uint16_t mt_gid_oid,
				    struct uci_session *session, const struct uci_blk *payload,
				    void *user_data);

	/**
	 * @get_app_config: Get session configuration.
	 * @uci: uci context needed to send uci messages.
	 * @mt_gid_oid: Union of message type, group identifier and opcode
	 * identifier.
	 * @payload: The whole uci message to process.
	 * @session_info: The session concerned by the call.
	 * @user_data: user data previously passed in
	 * uci_backend_manager_register.
	 *
	 * This is a mandatory Ops, if NULL uci_backend_manager_register will
	 * fail.
	 *
	 * Return: QERR_SUCCESS or an error.
	 */
	enum qerr (*get_app_config)(struct uci *uci, uint16_t mt_gid_oid,
				    const struct uci_blk *payload, struct uci_session *session_info,
				    void *user_data);

	/**
	 * @get_state: Get session state.
	 * @uci: uci context needed to send uci messages.
	 * @mt_gid_oid: Union of message type, group identifier and opcode
	 * identifier.
	 * @session_handle: The handle for the session.
	 * @user_data: user data previously passed in
	 * uci_backend_manager_register.
	 *
	 * This is a mandatory Ops, if NULL uci_backend_manager_register will
	 * fail.
	 *
	 * Return: QERR_SUCCESS or an error.
	 */
	enum qerr (*get_state)(struct uci *uci, uint16_t mt_gid_oid, uint32_t session_handle,
			       void *user_data);

	/**
	 * @get_current_state: Get session current state.
	 * @session_handle: The handle for the session.
	 * @user_data: user data previously passed in
	 * uci_backend_manager_register.
	 *
	 * This is a mandatory Ops, if NULL uci_backend_manager_register will
	 * fail.
	 *
	 * Return: Current state or a negative error.
	 */
	int (*get_current_state)(uint32_t session_handle, void *user_data);

	/**
	 * @get_ranging_count: Get ranging count.
	 * @uci: uci context needed to send uci messages.
	 * @mt_gid_oid: Union of message type, group identifier and opcode
	 * identifier.
	 * @session_handle: The handle for the session.
	 * @user_data: user data previously passed in
	 * uci_backend_manager_register.
	 *
	 * This is a mandatory Ops, if NULL uci_backend_manager_register will
	 * fail.
	 *
	 * Return: QERR_SUCCESS or an error.
	 */
	enum qerr (*get_ranging_count)(struct uci *uci, uint16_t mt_gid_oid,
				       uint32_t session_handle, void *user_data);
};

/**
 * struct uci_session_controller - Definition of controller (or a sub-backend).
 */
struct uci_session_controller {
	/**
	 * @session_ops: Session ops of this controller.
	 */
	struct uci_session_controller_ops *session_ops;
	/**
	 * @user_data: Data to pass to controller when calling an operation.
	 */
	void *user_data;
	/**
	 * @next: Pointer to next controller, or NULL if this
	 * is the last one. Filled at registration.
	 */
	struct uci_session_controller *next;
	/**
	 * @stypes: Type of sessions associated with this controller.
	 */
	enum quwbs_fbs_session_type stypes[MAX_NUMBER_OF_STYPES];
	/**
	 * @num_stypes: Number of session types associated with this controller.
	 */
	uint8_t num_stypes;
};

/**
 * uci_backend_manager_register() - Register a controller.
 * @manager: backend context
 * @session_ops: Controller session ops to register.
 * @stypes: Type of sessions to associate with ops.
 * @num_stypes: Number of session types associated with this controller.
 * @user_data: Data to give ops when calling it, used to pass along their
 * context.
 *
 * Return: QERR_SUCCESS or an error code.
 */
enum qerr uci_backend_manager_register(struct uci_backend_manager *manager,
				       struct uci_session_controller_ops *session_ops,
				       enum quwbs_fbs_session_type stypes[], size_t num_stypes,
				       void *user_data);

/**
 * uci_backend_manager_register_default() - Register a default controller.
 * @manager: backend context
 * @session_ops: Controller session ops to register.
 * @user_data: Data to give ops when calling it, used to pass along their
 * context.
 *
 * The default controller will be used for un-handled session_type.
 * Return: QERR_SUCCESS or an error code.
 */
enum qerr uci_backend_manager_register_default(struct uci_backend_manager *manager,
					       struct uci_session_controller_ops *session_ops,
					       void *user_data);

/**
 * uci_backend_manager_unregister() - Remove all handlers associated to a
 * session type.
 * @manager: backend context
 * @session_ops: ops of the controller to remove.
 */
void uci_backend_manager_unregister(struct uci_backend_manager *manager,
				    struct uci_session_controller_ops *session_ops);

/**
 * uci_backend_manager_unregister_default() - Remove all handlers associated to
 * the default controller.
 * @manager: backend context
 */
void uci_backend_manager_unregister_default(struct uci_backend_manager *manager);

/**
 * uci_backend_manager_disable_uwb() - Disable uwb because it is used by another
 * stack.
 * @manager: context.
 * @disable: true to disable, false to enable.
 *
 * On some hardware two stacks have access to the chipset. This is used to
 * disable this stack when necessary. It is enabled by default.
 */
void uci_backend_manager_disable_uwb(struct uci_backend_manager *manager, bool disable);

/**
 * struct uci_session - Holds information on an initialized session.
 */
struct uci_session {
	/**
	 * @next: Next element in list.
	 * NOTE: Private member
	 */
	struct uci_session *next; /* Private field, do not use */
	/**
	 * @destructor: Function to call to release all memory for this struct.
	 */
	void (*destructor)(struct uci_session *);
	/**
	 * @handle: Unique ID.
	 * NOTE: Private member
	 */
	uint32_t handle; /* Private field, do not overwrite */
	/**
	 * @type: Type as given at init.
	 * NOTE: Private member
	 */
	enum quwbs_fbs_session_type type;
	/**
	 * @controller: Controller associated to this session.
	 * NOTE: Private member
	 */
	struct uci_session_controller *controller;
	/**
	 * @n_measurements: Cached current number of measurements.
	 */
	int n_measurements;
	/**
	 * @nb_range: Number of range measurements.
	 */
	uint8_t nb_range;
	/**
	 * @nb_elevation: Number of elevation measurements.
	 */
	uint8_t nb_elevation;
	/**
	 * @nb_azimuth: Number of azimuth measurements.
	 */
	uint8_t nb_azimuth;
	/**
	 * @aoa_result_req: The type of aoa report requested.
	 */
	uint8_t aoa_result_req;
};

/**
 * uci_session_destructor() - Simple destructor that just frees.
 * @session: Session to destroy.
 */
static inline void uci_session_destructor(struct uci_session *session)
{
	qfree(session);
}

/**
 * uci_session_add() - Add a new session.
 * @manager: Session manager to use.
 * @session_handle: Session handle.
 * @session_type: Session type.
 * @session_ops: ops of the controller, used to restrict access to the session.
 * @destructor: Function to call to release all memory for this struct.
 * @session: Session to add.
 *
 * Return: UCI_STATUS_OK or UCI_STATUS_FAILED.
 */
enum uci_status_code uci_session_add(struct uci_backend_manager *manager, uint32_t session_handle,
				     enum quwbs_fbs_session_type session_type,
				     struct uci_session_controller_ops *session_ops,
				     void (*destructor)(struct uci_session *),
				     struct uci_session *session);

/**
 * uci_session_remove() - Remove and destroy a session.
 * @manager: Session manager to use.
 * @session_handle: Session handle.
 * @session_ops: Ops of the controller.
 *
 * Return: QERR_SUCCESS or
 *         QERR_EINVAL (session doesn't exist for this controller).
 */
enum qerr uci_session_remove(struct uci_backend_manager *manager, uint32_t session_handle,
			     struct uci_session_controller_ops *session_ops);

/**
 * uci_session_get() - Get a session.
 * @manager: Session manager to use.
 * @session_handle: Session handle.
 * @session_ops: Ops of the controller.
 * @session: Pointer to session structure (can be NULL if not needed).
 *
 * Return: UCI_STATUS_OK, UCI_STATUS_ERROR_SESSION_NOT_EXIST or UCI_STATUS_FAILED.
 */
enum uci_status_code uci_session_get(struct uci_backend_manager *manager, uint32_t session_handle,
				     struct uci_session_controller_ops *session_ops,
				     struct uci_session **session);

/**
 * uci_session_count() - Get sessions number.
 * @manager: Session manager to use.
 * @type: Type to count sessions for, or UCI_SESSION_TYPE_ALL for all types.
 *
 * Return: number of sessions in manager.
 */
int uci_session_count(struct uci_backend_manager *manager, enum quwbs_fbs_session_type type);
