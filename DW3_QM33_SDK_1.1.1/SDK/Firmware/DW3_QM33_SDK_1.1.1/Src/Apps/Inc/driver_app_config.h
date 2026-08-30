/**
 * @file      driver_app_config.h
 *
 * @brief     Interface for application config driver
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include <stdbool.h>
#include "deca_device_api.h"
#if defined(CLI_BUILD)
#include "fira_app_config.h"
#include "app.h"
#endif

#define APP_DEFAULT_PDOA_MODE DWT_PDOA_M1
#define VALID_VERSION         (0x00000001)
struct sts_config_s
{
    dwt_sts_cp_key_t stsKey; /* AES Key to be used to set the STS. */
    dwt_sts_cp_iv_t stsIv;   /* AES IV to be used to set the initial IV. */
    int stsInteropMode;      /* Configuration to STS/IV : value 0 = dynamic STS, 1 = fixed STS. */
};
typedef struct sts_config_s sts_config_t;

struct dwt_app_config_s
{
    dwt_config_t dwt_config; /* Standard Qorvo driver config. */
    sts_config_t sts_config;
#if defined(CLI_BUILD)
    fira_param_t fira_config;
    const app_definition_t *app;
#endif
    uint32_t version;
    bool comm_uart_allowed;
    uint8_t xtal_trim;
};
typedef struct dwt_app_config_s dwt_app_config_t;

/**
 * @brief Configuration for listener application.
 *
 * @return Pointer to dwt_app_config.
 */
dwt_app_config_t *get_app_dwt_config(void);

/**
 * @brief Configuration for listener application.
 *
 * @return Pointer to dwt_config.
 */
dwt_config_t *get_dwt_config(void);

/**
 * @brief Configuration for sts key and iv.
 *
 * @return Pointer to sts_config.
 */
sts_config_t *get_sts_config(void);

/**
 * @brief Get pdoa mode.
 *
 * @return mode
 */
uint8_t get_pdoa_mode(void);

/**
 * @brief Function for checking if the communication via UART is allowed.
 *
 * @return true  The communication via UART is allowed.
 * @return false The communication via UART is not allowed.
 */
bool is_uart_allowed(void);

/**
 * @brief Function for allowing the communication via UART.
 *
 * @param allow True to allow the communication via UART, false to disable.
 */
void set_uart_allowed(bool allow);


#if defined(CLI_BUILD)
/**
 * @brief Configuration for fira parameters.
 *
 * @return Pointer to fira_param_t.
 */
fira_param_t *get_fira_config(void);
#endif
