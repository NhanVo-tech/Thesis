/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "quwbs/fbs/defs.h"
#include "uwbmac/uwbmac_msg.h"

#include <net/fbs_region_nl.h>
#include <net/fbs_region_params.h>
#include <qerr.h>
#include <stddef.h>
#include <stdint.h>
#include <uwbmac/helper.h>

struct uwbmac_context;
struct fbs_helper_session_status_ntf;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * typedef fbs_helper_reader_cb - Callback to parse the response.
 * @msg: Response message to parse.
 * @reader_ctx: Reader context.
 *
 * Return: QERR_SUCCESS on success, otherwise an error
 */
typedef enum qerr (*fbs_helper_reader_cb)(struct uwbmac_msg *msg, void *reader_ctx);

/**
 * typedef fbs_helper_device_status_ntf_cb() - Callback for device status ntf.
 * @cookie: Callback context given when opening fbs_helper.
 * @state: Device state.
 */
typedef void (*fbs_helper_device_status_ntf_cb)(void *cookie, enum fbs_device_state state);

/**
 * typedef fbs_helper_session_status_ntf_cb() - Callback for session status ntf.
 * @cookie: Callback context given when opening fbs_helper.
 * @status: Session status notification notification.
 */
typedef void (*fbs_helper_session_status_ntf_cb)(
	void *cookie, const struct fbs_helper_session_status_ntf *status);

/**
 * typedef fbs_helper_other_ntf_cb() - Callback for other notifications.
 * @cookie: Callback context given when opening fbs_helper.
 * @id: Notification type identifier.
 * @msg: Notification message to decode.
 */
typedef void (*fbs_helper_other_ntf_cb)(void *cookie, enum fbs_call id, struct uwbmac_msg *msg);

/*
 * struct fbs_helper_filler - Context used by the filler handler.
 */
struct fbs_helper_filler {
	uint32_t session_handle;
	helper_msg_filler_cb session_cb;
	const void *session_ctx;
	helper_msg_filler_cb params_cb;
	const void *params_ctx;
};

struct fbs_helper_msg {
	struct helper *helper;
	struct uwbmac_msg root;
	struct uwbmac_msg request_nest;
	struct uwbmac_msg params_nest;
};

/**
 * struct fbs_helper - FSB helper context.
 *
 * This structure won't be public in the future.
 */
struct fbs_helper {
	/** @helper: Context of the helper. */
	struct helper *helper;
	/** @cookie: Context for callbacks, given in fbs_helper_open. */
	void *cookie;
	/** @device_status_ntf_cb: Callback for device status notification (can be NULL). */
	fbs_helper_device_status_ntf_cb device_status_ntf_cb;
	/** @session_status_ntf_cb: Callback for session status notification (can be NULL). */
	fbs_helper_session_status_ntf_cb session_status_ntf_cb;
	/** @other_ntf_cb: Callback for other notifications (can be NULL). */
	fbs_helper_other_ntf_cb other_ntf_cb;
};

/**
 * struct fbs_session_init_rsp - Fira FBS_CALL_SESSION_INIT response content.
 */
struct fbs_session_init_rsp {
	/** @status_code: Status code. */
	enum quwbs_fbs_status status_code;
	/** @session_handle: Session handle. */
	uint32_t session_handle;
};

/**
 * struct fbs_ranging_count_rsp - Fira FBS_CALL_RANGING_COUNT response content.
 */
struct fbs_ranging_count_rsp {
	/** @status_code: Status code. */
	enum quwbs_fbs_status status_code;
	/** @ranging_count: Number of ranging round done from the START command. */
	uint32_t ranging_count;
};

/**
 * struct fbs_helper_session_status_ntf - Context used with a STATUS notification.
 *
 * See SESSION_STATUS_NTF in UCI FiRA Standard.
 */
struct fbs_helper_session_status_ntf {
	/** @session_handle: Session handle value. */
	uint32_t session_handle;
	/** @state: Session state. */
	enum quwbs_fbs_session_state state;
	/** @reason_code: Reason code. */
	enum quwbs_fbs_reason_code reason_code;
};

/**
 * fbs_helper_open() - Initialize the fbs_helper context.
 * @ctx: FBS context to initialize.
 * @uwbmac: Uwbmac context.
 * @cookie: Custom context link the callbacks.
 * @scheduler_name: Address of the scheduler name.
 * @region_name: Address of the region name.
 * @region_id: Region ID to be called.
 * @buffer_size: Size of shared memory used to prepare all messages.
 *
 * Return: QERR_SUCCESS on success, otherwise an error.
 */
enum qerr fbs_helper_open(struct fbs_helper *ctx, struct uwbmac_context *uwbmac, void *cookie,
			  const char *scheduler_name, const char *region_name, int region_id,
			  size_t buffer_size);

/**
 * fbs_helper_close() - Close and reset the fbs_helper context.
 * @ctx: FBS helper context to close.
 */
void fbs_helper_close(struct fbs_helper *ctx);

/**
 * fbs_helper_set_device_status_ntf_cb() - Set the device status ntf callback.
 * @ctx: FBS helper context.
 * @cb: Callback for all device status notifications (can be NULL).
 *
 * Return: QERR_SUCCESS on success, on error otherwise.
 */
enum qerr fbs_helper_set_device_status_ntf_cb(struct fbs_helper *ctx,
					      fbs_helper_device_status_ntf_cb cb);

/**
 * fbs_helper_set_session_status_ntf_cb() - Set the session status ntf callback.
 * @ctx: FBS helper context.
 * @cb: Callback for all session status notifications (can be NULL).
 *
 * Return: QERR_SUCCESS on success, on error otherwise.
 */
enum qerr fbs_helper_set_session_status_ntf_cb(struct fbs_helper *ctx,
					       fbs_helper_session_status_ntf_cb cb);

/**
 * fbs_helper_set_other_ntf_cb() - Set the other ntf callback.
 * @ctx: FBS helper context.
 * @cb: Callback for notifications not take in charge by fbs_helper.
 */
void fbs_helper_set_other_ntf_cb(struct fbs_helper *ctx, fbs_helper_other_ntf_cb cb);

/**
 * fbs_helper_set_region() - set scheduler and region.
 * @ctx: FBS helper context to use.
 *
 * Return: QERR_SUCCESS on success.
 */
enum qerr fbs_helper_set_region(struct fbs_helper *ctx);

/**
 * fbs_helper_send_msg_by_cb() - Send the message to the MAC layer.
 * @ctx: FBS helper context used for routing.
 * @call_id: Call identifier forwarded to the MAC.
 * @reply: Reply message (can be NULL) (WIP: see struct message).
 * @filler: Filler's context to follow to fill the request message.
 *
 * Return: QERR_SUCCESS on success.
 */
enum qerr fbs_helper_send_msg_by_cb(struct fbs_helper *ctx, enum fbs_call call_id,
				    struct uwbmac_msg *reply,
				    const struct fbs_helper_filler *filler);

/**
 * fbs_helper_init_set_params_msg() - Initializes a set session parameters.
 * @ctx: FBS helper context.
 * @msg: The FBS message context to initializes.
 * @session_handle: Session handle.
 *
 * Return: QERR_SUCCESS on success, otherwise an error.
 */
enum qerr fbs_helper_init_set_params_msg(struct fbs_helper *ctx, struct fbs_helper_msg *msg,
					 uint32_t session_handle);

/**
 * fbs_helper_send_set_params_msg() - Closes the message and send it to MAC.
 * @msg: The FBS message context to send.
 *
 * On success, the message is automatically uninit.
 *
 * Return: QERR_SUCCESS on success, otherwise an error.
 */
enum qerr fbs_helper_send_set_params_msg(struct fbs_helper_msg *msg);

/**
 * fbs_helper_uninit_set_params_msg() - Resets an initialized message.
 * @msg: The FBS message context to reset.
 *
 * Return: QERR_SUCCESS on success, otherwise an error.
 */
enum qerr fbs_helper_uninit_set_params_msg(struct fbs_helper_msg *msg);

/**
 * fbs_helper_send_session_init() - Send SESSION_INIT command.
 * @ctx: FBS helper context.
 * @session_id: Session identifier value.
 * @session_type: Session type value.
 * @rsp: Session init response message information.
 *
 * Return: QERR_SUCCESS on success.
 */
enum qerr fbs_helper_send_session_init(struct fbs_helper *ctx, uint32_t session_id,
				       enum quwbs_fbs_session_type session_type,
				       struct fbs_session_init_rsp *rsp);

/**
 * fbs_helper_send_basic_session_cmd() - Send basic session command.
 * @ctx: FBS Helper context.
 * @session_handle: Session handle.
 * @call_id: Call identifier forwarded to the MAC.
 * @reply: Reply message (can be NULL) (WIP: see struct message).
 *
 * A basic command contains only the session_handle and the call_id in the
 * message.
 *
 * WIP: The return value type will be enum qerr as soon the
 * fira_helper_start_session doesn't needs of an int.
 *
 * Return: 0 or positive value on success, negative value on error.
 */
enum qerr fbs_helper_send_basic_session_cmd(struct fbs_helper *ctx, uint32_t session_handle,
					    enum fbs_call call_id, struct uwbmac_msg *reply);

/**
 * fbs_helper_get_capabilities() - Get the FiRa region capabilities.
 * @ctx: FBS Helper context.
 * @reader_cb: Reader callback.
 * @reader_ctx: Context of the reader callback (can be NULL).
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fbs_helper_get_capabilities(struct fbs_helper *ctx, fbs_helper_reader_cb reader_cb,
				      void *reader_ctx);

/**
 * fbs_helper_get_ranging_count() - Gets the number of ranging round that have been attempted.
 * @ctx: FBS Helper context.
 * @session_handle: Session handle value.
 * @rsp: Ranging count response message information.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fbs_helper_get_ranging_count(struct fbs_helper *ctx, uint32_t session_handle,
				       struct fbs_ranging_count_rsp *rsp);

/**
 * fbs_helper_session_get_state() - Gets the session's state.
 * @ctx: FBS Helper context.
 * @session_handle: Session handle value.
 * @state: Pointer updated with the session state retrieved.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fbs_helper_session_get_state(struct fbs_helper *ctx, uint32_t session_handle,
				       enum quwbs_fbs_session_state *state);

/**
 * fbs_helper_session_get_count() - Get sessions count, the number of active and inactive sessions.
 * @ctx: FBS helper context.
 * @count: Pointer updated with the number of sessions found.
 *
 * Return: QERR_SUCCESS on success, an error otherwise.
 */
enum qerr fbs_helper_session_get_count(struct fbs_helper *ctx, uint32_t *count);

#ifdef __cplusplus
}
#endif
