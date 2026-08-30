/**
 * @file      HAL_watchdog.c
 *
 * @brief     Hardware abstraction layer for watchdog
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "HAL_watchdog.h"

#include "nrf_drv_wdt.h"

nrf_drv_wdt_channel_id m_channel_id;
uint32_t wdt_reset_cnt = 0;

static void wdt_event_handler(void)
{
    /* NOTE: The max amount of time we can spend in WDT interrupt is two cycles
     * of 32768[Hz] clock - after that, reset occurs. */
    wdt_reset_cnt = wdt_reset_cnt + 1;
}

static void wdt_refresh(void)
{
    nrf_drv_wdt_channel_feed(m_channel_id);
}

static void wdt_init(int ms)
{
    ret_code_t err_code;

    nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;

    config.reload_value = ms;
    err_code = nrf_drv_wdt_init(&config, wdt_event_handler);
    APP_ERROR_CHECK(err_code);
    err_code = nrf_drv_wdt_channel_alloc(&m_channel_id);
    APP_ERROR_CHECK(err_code);
    nrf_drv_wdt_enable();
}

const struct hal_watchdog_s Watchdog = {
    .init = &wdt_init,
    .refresh = &wdt_refresh};
