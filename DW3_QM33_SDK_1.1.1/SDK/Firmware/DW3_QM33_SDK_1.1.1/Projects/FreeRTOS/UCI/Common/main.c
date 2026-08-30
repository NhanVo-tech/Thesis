/**
 * @file      main.c
 *
 * @brief     FreeRTOS main
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "qos.h"
#include "app.h"
#include "thisBoard.h"
#include "controlTask.h"
#include "HAL_error.h"
#include "flushTask.h"
#include "defaultTask.h"
#include <stddef.h>
#if CONFIG_LOG
#include <log_processing.h>
#endif

int main(void)
{
    BoardInit();
#if CONFIG_LOG
    /* Start Log processing task, right after configuring clock sources. */
    create_log_processing_task();
#endif

    AppConfigInit();
    board_interface_init();

    /* Initialize interface output thread. */
    FlushTaskInit();

    /* Initialize interface input thread. */
    ControlTaskInit();

    /* Start UCI thread. */
    extern const app_definition_t helpers_uci_node[];
    AppSet(helpers_uci_node);
    AppGet()->helper(NULL);

    /* Start scheduler */
    qos_start();

    /* We should never get here as control is now taken by the scheduler. */
    while (1)
    {
    }

    return 0;
}
