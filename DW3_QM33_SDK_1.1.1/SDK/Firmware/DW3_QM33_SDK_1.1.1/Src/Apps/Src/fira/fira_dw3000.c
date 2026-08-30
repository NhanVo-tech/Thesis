/**
 * @file      fira_dw3000.c
 *
 * @brief     FiRa for DW3000
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <stdio.h>

#include "common_fira.h"
#include "llhw.h"
#include "qplatform.h"

extern struct l1_config_platform_ops l1_config_platform_ops;

enum qerr fira_uwb_mcps_init(struct uwbmac_context **uwbmac_ctx)
{
    enum qerr r;

    r = qplatform_init();
    if (r != QERR_SUCCESS)
        return r;

    r = l1_config_init(&l1_config_platform_ops);
    if (r != QERR_SUCCESS)
        goto deinit_qplatform;

    r = llhw_init();
    if (r != QERR_SUCCESS)
        goto deinit_l1_config;

    r = uwbmac_init(uwbmac_ctx);

    if (r == QERR_SUCCESS)
        goto exit;

    llhw_deinit();
deinit_l1_config:
    l1_config_deinit();
deinit_qplatform:
    qplatform_deinit();
exit:
    return r;
}

void fira_uwb_mcps_deinit(struct uwbmac_context *uwbmac_ctx)
{
    llhw_deinit();
    l1_config_deinit();
    qplatform_deinit();
}
