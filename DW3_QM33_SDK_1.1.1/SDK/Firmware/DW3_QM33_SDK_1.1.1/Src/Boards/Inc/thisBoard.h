/**
 * @file      thisBoard.h
 *
 * @brief     Interface for dev board
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#define BOARD_WD_INIT_TIME 60000

void BoardInit(void);
void board_interface_init(void);
void uwb_init(void);
