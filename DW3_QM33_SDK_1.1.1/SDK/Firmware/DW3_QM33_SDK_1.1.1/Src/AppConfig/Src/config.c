/*
 * @file      config.c
 *
 * @brief     Module to save/load/restore configuration
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include <stdint.h>
#include <string.h>

#include "appConfig.h"
#include "nrf_nvmc.h"
#include "driver_app_config.h"

#define INVALID_VERSION_ALL_ONES (0xFFFFFFFF)

extern uint8_t __config_entry_start[];
extern uint8_t __config_entry_end[];

extern uint8_t __fconfig_start[];

extern uint8_t __rconfig_start[];
extern uint8_t __rconfig_end[];

__attribute__((section(".fconfig"))) const dwt_app_config_t persistent_app_config;

static bool auto_restore = false;

/**
 * @brief Copy parameters from NVM to RAM structure.
 * Assumes that memory model in the MCU of .text and .bss are the same.
 */
void load_bssConfig(bool initStatus)
{
    dwt_app_config_t *app_config;
    memcpy((uint8_t *)&__rconfig_start, (uint8_t *)&__fconfig_start, sizeof(dwt_app_config_t));
    app_config = (dwt_app_config_t *)&__rconfig_start;
    if (app_config->version < VALID_VERSION || app_config->version == INVALID_VERSION_ALL_ONES)
    {
        restore_bssConfig(initStatus);
    }
}

/**
 * @brief Copy parameters from default RAM section to RAM structure.
 * Assumes that memory model in the MCU of .text and .bss are the same
 */
void restore_bssConfig(bool initStatus)
{
    /* Copy default to RAM ( listener/fira params and uart config ). */
    for (uint32_t *ptr = (uint32_t *)&__config_entry_start; ptr < (uint32_t *)&__config_entry_end; ptr++)
    {
        void (*restore)(void) = (void (*)(void))(*ptr);
        restore();
    }
    save_bssConfig();
}

/**
 * @brief Save pNewRamParametersBlock to FCONFIG_ADDR.
 * @return  _NO_ERR for success and error_e code otherwise.
 */
error_e save_bssConfig(void)
{
#if defined(CLI_BUILD)
    dwt_app_config_t *all_params = get_app_dwt_config();
    uint32_t num_words = sizeof(dwt_app_config_t) / sizeof(uint32_t);

    /* Align on words */
    if (sizeof(dwt_app_config_t) & (sizeof(uint32_t) - 1))
        num_words += 1;
    nrf_nvmc_page_erase((uint32_t)&__fconfig_start);
    nrf_nvmc_write_words((uint32_t)&__fconfig_start, (const uint32_t *)all_params, num_words);
#endif
    return (_NO_ERR);
}

bool is_auto_restore_bssConfig(void)
{
    return auto_restore;
}
