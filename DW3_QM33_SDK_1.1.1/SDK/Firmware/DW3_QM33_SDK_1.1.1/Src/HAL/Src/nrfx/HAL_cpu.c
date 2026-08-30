/**
 * @file      HAL_cpu.c
 *
 * @brief     Common functionalities for nrf52 based boards.
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#include "HAL_cpu.h"
#include "nrf.h"
#include "nrf_erratas.h"
#include "boards.h"

void handle_cpu_protect(void)
{
#ifdef CONFIG_SOC_PROTECT
    if ((NRF_UICR->APPROTECT & UICR_APPROTECT_PALL_Msk) != (UICR_APPROTECT_PALL_Enabled << UICR_APPROTECT_PALL_Pos))
    {
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {}
        NRF_UICR->APPROTECT = ((NRF_UICR->APPROTECT & ~((uint32_t)UICR_APPROTECT_PALL_Msk)) | (UICR_APPROTECT_PALL_Enabled << UICR_APPROTECT_PALL_Pos));
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {}
        NVIC_SystemReset();
    }
#else
    /* Latest chip revision sets different values than old revisions. */
    const uint32_t disable_val = nrf52_errata_249() ? UICR_APPROTECT_PALL_HwDisabled : UICR_APPROTECT_PALL_Disabled;

    if ((NRF_UICR->APPROTECT & UICR_APPROTECT_PALL_Msk) != disable_val)
    {
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {}
        NRF_UICR->APPROTECT = ((NRF_UICR->APPROTECT & ~((uint32_t)UICR_APPROTECT_PALL_Msk)) | disable_val);
        NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
        while (NRF_NVMC->READY == NVMC_READY_READY_Busy)
        {}
        NVIC_SystemReset();
    }
#endif
}

void FPU_IRQHandler(void)
{
    const uint32_t FPU_EXCEPTION_MASK = 0x0000009F;
    uint32_t *fpscr = (uint32_t *)(FPU->FPCAR + 0x40);
    (void)__get_FPSCR();

    *fpscr = *fpscr & ~(FPU_EXCEPTION_MASK);
}

void handle_fpu_irq(void)
{
    NVIC_SetPriority(FPU_IRQn, APP_IRQ_PRIORITY_LOW);
    NVIC_EnableIRQ(FPU_IRQn);
}
