/**
 * @file      task_uci.h
 *
 * @brief     Interface for UCI task
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define UCI_DATA_IN  2
#define UCI_DATA_OUT 4

#include <stdint.h>
#include <stdbool.h>

void uci_helper(void const *arg);
void uci_terminate(void);
void uci_interface_select();
void uci_process_terminate(void);
bool uci_sw_reset(void);
int uci_open_backends(void);
void uci_close_backends(void);

#ifdef __cplusplus
}
#endif
