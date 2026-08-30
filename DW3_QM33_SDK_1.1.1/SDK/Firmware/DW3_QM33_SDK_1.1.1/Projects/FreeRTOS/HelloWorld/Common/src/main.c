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
#include "HAL_error.h"
#include "HAL_cpu.h"
#include "hello_world.h"
#include "qlog.h"
#include "nrf_drv_clock.h"
#if CONFIG_LOG
#include "log_processing.h"
#endif

int main(void)
{
    handle_cpu_protect();

    ret_code_t ret = nrf_drv_clock_init();
    if ((ret != NRF_SUCCESS) && (ret != NRF_ERROR_MODULE_ALREADY_INITIALIZED))
    {
        error_handler(1, _ERR);
    }

    /* Start LFCLK for proper operation of the RTC. */
    nrfx_clock_lfclk_start();
    while (!nrfx_clock_lfclk_is_running())
        ;

#if CONFIG_LOG
    create_log_processing_task();
#endif

    error_e err = hello_world_init();
    if (err != _NO_ERR)
    {
        error_handler(1, err);
    }

    /* Start scheduler. */
    qos_start();

    /* This point should never be reached, as control is now taken by the scheduler. */
    while (1)
    {
    }

    return 0;
}
