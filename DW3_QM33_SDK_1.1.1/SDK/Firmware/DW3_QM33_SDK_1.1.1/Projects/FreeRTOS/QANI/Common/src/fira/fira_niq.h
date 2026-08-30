/**
 *  @file     fira_niq.h
 *
 *  @brief    Fira for Qorvo Nearby Interaction
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include "niq.h"

void StartUWB(fira_device_configure_t *config, void *user_ctx);
void StopUWB(uint32_t session_id, void *user_ctx);
void PrepareUwbCalibration(void);
error_e CreateQaniTask(void);
