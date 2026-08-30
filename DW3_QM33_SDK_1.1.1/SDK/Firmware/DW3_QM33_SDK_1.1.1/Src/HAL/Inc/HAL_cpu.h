/**
 * @file      nrf52_board.h
 *
 * @brief     Common functionalities for nrf52 based boards.
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

/* Enable or disable APPROTECT (Access Port Protection) depending on a compilation flag. */
void handle_cpu_protect(void);

/* Enable IRQ for FPU exceptions and clear pending flag when occurred. This is a workaround of nRF SDK issue which prevents
CPU from going to sleep. */
void handle_fpu_irq(void);
