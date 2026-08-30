/**
 * @file      fira_app_config.h
 *
 * @brief     Default FiRa app config file for NVM initialization
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include "uwbmac/fira_helper.h"

typedef enum
{
    FIRA_APP_INITF = 0,
    FIRA_APP_RESPF = 1,
    FIRA_APP_NONE = 2
} fira_app_type_t;

typedef enum
{
    FIRA_APP_CONFIG_DEFAULT,
    FIRA_APP_CONFIG_USER,
    FIRA_APP_CONFIG_SAVED
} fira_app_config_state_t;

struct fira_param_s
{
    uint32_t session_id;
    uint16_t short_addr;
    struct session_parameters session;
    struct controlees_parameters controlees_params;
    fira_app_type_t app_type;
    fira_app_config_state_t config_state;
};
typedef struct fira_param_s fira_param_t;

fira_param_t *get_fira_config(void);
void set_fira_config(fira_param_t *fira_config);
