/**
 * @file      fira_default_params.h
 *
 * @brief     Header file containing FiRa default parameters
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#define FIRA_DEFAULT_RFRAME_CONFIG         FIRA_RFRAME_CONFIG_SP3
#define FIRA_DEFAULT_SFD_ID                FIRA_SFD_ID_2
#define FIRA_DEFAULT_SLOT_DURATION_RSTU    2400
#define FIRA_DEFAULT_BLOCK_DURATION_MS     200
#define FIRA_DEFAULT_ROUND_DURATION_SLOTS  25
#define FIRA_DEFAULT_RANGING_ROUND_USAGE   FIRA_RANGING_ROUND_USAGE_DSTWR_DEFERRED
#define FIRA_DEFAULT_SESSION_ID            42
#define FIRA_DEFAULT_MULTI_NODE_MODE       FIRA_MULTI_NODE_MODE_UNICAST
#define FIRA_DEFAULT_ROUND_HOPPING         false
#define FIRA_DEFAULT_NUM_OF_CONTROLEES     1
#define FIRA_DEFAULT_CONTROLLER_SHORT_ADDR 0x0
#define FIRA_DEFAULT_CONTROLLEE_SHORT_ADDR 0x1
/* Report ToF. */
#define FIRA_DEFAULT_RESULT_REPORT_CONFIG 0x1
/* Result report phase, skip ranging control phase. */
#define FIRA_DEFAULT_RANGING_ROUND_CONTROL 0x3
