/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include <qerr.h>
#include <stddef.h>
#include <uwbmac/uwbmac.h>

struct uwbmac_context;
struct uwbmac_msg;
struct helper;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * typedef helper_msg_filler_cb - Callback to fill the request msg.
 * @filler_ctx: Custom filler context.
 * @msg: Message to fill.
 *
 * Return: QERR_SUCCESS on success, otherwise an error
 */
typedef enum qerr (*helper_msg_filler_cb)(const void *filler_ctx, struct uwbmac_msg *msg);

/**
 * helper_new() - Creates a new helper context.
 * @uwbmac: Uwbmac context.
 * @cb: Callback for messages from the MAC.
 * @cb_ctx: Custom context link the callback.
 * @scheduler_name: Address of the scheduler name.
 * @region_name: Address of the region name.
 * @region_id: Region ID to be called.
 * @buffer_size: Size of shared memory used to prepare all messages.
 *
 * Return: An helper context or NULL on error.
 */
struct helper *helper_new(struct uwbmac_context *uwbmac, uwbmac_call_region_cb cb, void *cb_ctx,
			  const char *scheduler_name, const char *region_name, int region_id,
			  size_t buffer_size);

/**
 * helper_delete() - Deletes the helper context.
 * @ctx: Helper context to release.
 */
void helper_delete(struct helper *ctx);

/**
 * helper_set_region() - Sets the scheduler and the region.
 * @ctx: Helper context used for message routing.
 *
 * Return: QERR_SUCCESS on success, otherwise an error
 */
enum qerr helper_set_region(struct helper *ctx);

/**
 * helper_init_msg() - Initializes the message.
 * @ctx: Helper context used for message routing.
 * @msg: Message context to initialise.
 *
 * Return: QERR_SUCCESS on success, otherwise an error
 */
enum qerr helper_init_msg(struct helper *ctx, struct uwbmac_msg *msg);

/**
 * helper_uninit_msg() - Uninitializes the message.
 * @ctx: Helper context used for message routing.
 * @msg: Message to cleanup.
 */
void helper_uninit_msg(struct helper *ctx, struct uwbmac_msg *msg);

/**
 * helper_send_msg() - Sends the message to the MAC layer.
 * @ctx: Helper context used for message routing.
 * @msg: Message to send.
 * @call_id: Call identifier forwarded to the MAC.
 * @reply: Reply message to be updated (WIP: see structured message).
 *
 * Return: QERR_SUCCESS on success, otherwise an error
 */
enum qerr helper_send_msg(struct helper *ctx, const struct uwbmac_msg *msg, int call_id,
			  struct uwbmac_msg *reply);

/**
 * helper_send_msg_by_cb() - Sends the message to the MAC layer.
 * @ctx: Helper context used for message routing.
 * @call_id: Call identifier forwarded to the MAC.
 * @reply: Reply message to be updated (WIP: see structured message).
 * @filler_cb: Filler function to write the message to be sent.
 * @filler_ctx: Filler's context to forward to the filler function(can be NULL).
 *
 * Return: QERR_SUCCESS on success, otherwise an error
 */
enum qerr helper_send_msg_by_cb(struct helper *ctx, int call_id, struct uwbmac_msg *reply,
				helper_msg_filler_cb filler_cb, const void *filler_ctx);

#ifdef __cplusplus
}
#endif
