/**
 * @file      app.c
 *
 * @brief     Application configuration
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "app.h"
#include "HAL_error.h"
#include "appConfig.h"
#include "circular_buffer.h"
#include "cmd.h"
#if defined(CLI_BUILD)
#include "driver_app_config.h"
#endif

#if defined(CLI_BUILD)
const app_definition_t idle_app[] = {
    {"NONE", mIDLE, NULL, NULL, waitForCommand, command_parser}};
#define DEFAULT_APP idle_app
#else
const app_definition_t idle_app[] = {
    {"NONE", mIDLE, NULL, NULL, NULL, NULL}};
extern const app_definition_t helpers_uci_node[];
#define DEFAULT_APP helpers_uci_node
#endif

static const app_definition_t *default_app __attribute__((section(".rconfig"))) = DEFAULT_APP;

static const app_definition_t *app = idle_app;

static void restore_default_app(void)
{
    default_app = DEFAULT_APP;
}

__attribute__((section(".config_entry"))) const void (*p_restore_default_app)(void) = (const void *)&restore_default_app;

const app_definition_t *AppGet(void)
{
    return app;
}

void AppSet(const app_definition_t *_app)
{
    if (_app == NULL)
    {
        app = (app_definition_t *)&default_app;
    }
    else
    {
        app = _app;
    }
}

const app_definition_t *AppGetDefaultEvent(void)
{
#if defined(CLI_BUILD)

    dwt_app_config_t *app_dwt = get_app_dwt_config();
    return app_dwt->app;
#else
    return default_app;
#endif
}

error_e AppSetDefaultEvent(const app_definition_t *_app)
{
    default_app = _app;
    return save_bssConfig();
}

void AppConfigInit(void)
{
    /* Load the RAM Configuration parameters from NVM block. */
    load_bssConfig(0);
}
