/**
 * @file      uwbmac_helper_dw3000.c
 *
 * @brief     UWBMAC helper init and configuration
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "uwbmac_helper.h"
#include "llhw.h"
#include "qplatform.h"
#include "reporter.h"

extern struct l1_config_platform_ops l1_config_platform_ops;

/** @brief Setup the UWB chip. */
int uwbmac_helper_init_fira(void)
{
    int ret;
    (void)ret;

    ret = qplatform_init();
    if (ret != QERR_SUCCESS)
    {
        reporter_instance.print("qplatform_init failed", strlen("qplatform_init failed"));
        return ret;
    }

    ret = l1_config_init(&l1_config_platform_ops);
    if (ret != QERR_SUCCESS)
    {
        reporter_instance.print("l1_config_init failed", strlen("l1_config_init failed"));
        return ret;
    }

    ret = llhw_init();
    if (ret != QERR_SUCCESS)
    {
        reporter_instance.print("llhw_init failed", strlen("llhw_init failed"));
        return ret;
    }

    return 0;
}

void uwbmac_helper_deinit(void)
{
    llhw_deinit();
    l1_config_deinit();
    qplatform_deinit();
}
