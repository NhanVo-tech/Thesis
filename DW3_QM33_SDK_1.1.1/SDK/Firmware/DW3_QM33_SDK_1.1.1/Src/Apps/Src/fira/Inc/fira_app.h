/**
 *  @file     fira_app.h
 *
 *  @brief    Fira processes control
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void fira_terminate(void);
void fira_save_params(void);

bool fira_set_user_params(char *text, bool controller);

#ifdef __cplusplus
}
#endif
