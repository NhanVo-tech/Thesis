/**
 * @file      comm_helpers.h
 *
 * @brief     Communication helpers
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include "circular_buffer.h"

typedef void (*CommRxCallback)(struct cc_buff *buf);
