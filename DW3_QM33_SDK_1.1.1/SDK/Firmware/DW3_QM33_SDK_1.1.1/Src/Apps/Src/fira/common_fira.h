/**
 * @file      common_fira.h
 *
 * @brief     Interface for common FiRa functionalities
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "uwbmac/fira_helper.h"
#include "fira_app_config.h"

#ifdef QANI_BUILD
#include "qlog.h"
#define FIRA_LOGI(x, y) QLOGI("%s", x)
#define FIRA_LOGE(x, y) QLOGE("%s", x)
#else
#include "reporter.h"
#define FIRA_LOGI(x, y) reporter_instance.print((char *)x, y);
#define FIRA_LOGE(x, y) reporter_instance.print((char *)x, y);
#endif

/** @brief Data measurement structure. */
struct string_measurement
{
    /* Buffer of string measurement to allocate. */
    char *str;
    /* Length of measurement string. */
    uint16_t len;
};

void fira_show_params();
enum qerr fira_prepare_measurement_sequence(struct uwbmac_context *uwbmac_ctx, struct session_parameters *session, bool is_report_required);
enum qerr fira_set_session_parameters(struct fira_context *fira_context, uint32_t session_handle, struct session_parameters *session);
bool fira_scan_params(char *text, bool controller);

enum qerr fira_uwb_mcps_init(struct uwbmac_context **uwbmac_ctx);
void fira_uwb_mcps_deinit(struct uwbmac_context *uwbmac_ctx);

#ifdef __cplusplus
}
#endif
