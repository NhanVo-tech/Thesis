
/**
 * @file      driver_app_config.h
 *
 * @brief     Driver config file for NVM initialization
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "driver_app_config.h"

#include <string.h>
#include <stdint.h>

#if defined(CLI_BUILD)
#include "fira_default_params.h"
#include "fira_app_config.h"
#endif
#include "default_config.h"

#ifdef UCI_BUILD
#define COMM_UART_ALLOWED_DEFAULT true
#elif defined(USB_ENABLE)
#define COMM_UART_ALLOWED_DEFAULT false
#else
#define COMM_UART_ALLOWED_DEFAULT true
#endif

static const dwt_app_config_t dwt_app_config_flash_default = {
    .dwt_config.chan = APP_DEFAULT_CHANNEL,
    .dwt_config.txPreambLength = APP_DEFAULT_TXPREAMBLENGTH,
    .dwt_config.rxPAC = APP_DEFAULT_RXPAC,
    .dwt_config.txCode = APP_DEFAULT_PCODE,
    .dwt_config.rxCode = APP_DEFAULT_PCODE,
    .dwt_config.sfdType = APP_DEFAULT_NSSFD,
    .dwt_config.dataRate = APP_DEFAULT_DATARATE,
    .dwt_config.phrMode = APP_DEFAULT_PHRMODE,
    .dwt_config.phrRate = APP_DEFAULT_PHRRATE,
    .dwt_config.sfdTO = APP_DEFAULT_SFDTO,
    .dwt_config.stsMode = APP_DEFAULT_STS_MODE,
    .dwt_config.stsLength = APP_DEFAULT_STS_LENGTH,
    .dwt_config.pdoaMode = APP_DEFAULT_PDOA_MODE,
    .sts_config.stsKey.key0 = 0x14EB220FUL,
    .sts_config.stsKey.key1 = 0xF86050A8UL,
    .sts_config.stsKey.key2 = 0xD1D336AAUL,
    .sts_config.stsKey.key3 = 0x14148674UL,
    .sts_config.stsIv.iv0 = 0x1F9A3DE4UL,
    .sts_config.stsIv.iv1 = 0xD37EC3CAUL,
    .sts_config.stsIv.iv2 = 0xC44FA8FBUL,
    .sts_config.stsIv.iv3 = 0x362EEB34UL,
    .sts_config.stsInteropMode = 1L,
#if defined(CLI_BUILD)
    .fira_config.session_id = FIRA_DEFAULT_SESSION_ID,
    .fira_config.short_addr = FIRA_DEFAULT_CONTROLLER_SHORT_ADDR,
    .fira_config.controlees_params.n_controlees = FIRA_DEFAULT_NUM_OF_CONTROLEES,
    .fira_config.session.rframe_config = FIRA_DEFAULT_RFRAME_CONFIG,
    .fira_config.session.sfd_id = FIRA_DEFAULT_SFD_ID,
    .fira_config.session.slot_duration_rstu = FIRA_DEFAULT_SLOT_DURATION_RSTU,
    .fira_config.session.block_duration_ms = FIRA_DEFAULT_BLOCK_DURATION_MS,
    .fira_config.session.round_duration_slots = FIRA_DEFAULT_ROUND_DURATION_SLOTS,
    .fira_config.session.ranging_round_usage = FIRA_DEFAULT_RANGING_ROUND_USAGE,
    .fira_config.session.multi_node_mode = FIRA_DEFAULT_MULTI_NODE_MODE,
    .fira_config.session.round_hopping = FIRA_DEFAULT_ROUND_HOPPING,
    .fira_config.session.schedule_mode = FIRA_SCHEDULE_MODE_TIME_SCHEDULED,
    .fira_config.session.preamble_code_index = FIRA_DEFAULT_PREAMBLE_CODE_INDEX,
    .fira_config.session.phr_data_rate = FIRA_PRF_MODE_BPRF,
    .fira_config.session.channel_number = FIRA_DEFAULT_CHANNEL_NUMBER,
    .fira_config.session.result_report_config = FIRA_DEFAULT_RESULT_REPORT_CONFIG,
    .fira_config.session.ranging_round_control = FIRA_DEFAULT_RANGING_ROUND_CONTROL,
    .fira_config.config_state = FIRA_APP_CONFIG_DEFAULT,
    .fira_config.app_type = FIRA_APP_NONE,
#endif
    .version = VALID_VERSION,
    .comm_uart_allowed = COMM_UART_ALLOWED_DEFAULT,
    .xtal_trim = DEFAULT_XTAL_TRIM,
};

static dwt_app_config_t dwt_app_config_ram __attribute__((section(".rconfig"))) = {0};

dwt_app_config_t *get_app_dwt_config(void)
{
    return &dwt_app_config_ram;
}

dwt_config_t *get_dwt_config(void)
{
    return &dwt_app_config_ram.dwt_config;
}

sts_config_t *get_sts_config(void)
{
    return &dwt_app_config_ram.sts_config;
}
#if defined(CLI_BUILD)
fira_param_t *get_fira_config(void)
{
    return &dwt_app_config_ram.fira_config;
}
#endif
static void restore_dwt_app_default_config(void)
{
    memcpy(&dwt_app_config_ram, &dwt_app_config_flash_default, sizeof(dwt_app_config_ram));
}

uint8_t get_pdoa_mode(void)
{
    return dwt_app_config_ram.dwt_config.pdoaMode;
}

bool is_uart_allowed(void)
{
    return dwt_app_config_ram.comm_uart_allowed;
}

void set_uart_allowed(bool allow)
{
    dwt_app_config_ram.comm_uart_allowed = allow;
}

__attribute__((section(".config_entry"))) const void (*p_restore_driver_app_default_config)(void) = (const void *)&restore_dwt_app_default_config;
