/**
 * @file      reporter.h
 *
 * @brief     Interface for reporter
 *
 * @author    Qorvo Applications
 *
 * @copyright SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *            SPDX-License-Identifier: LicenseRef-QORVO-2
 *
 */

#pragma once

#include "deca_error.h"

struct reporter_s
{
    void (*init)(void);
    error_e (*print)(char *buff, int len);
};
typedef struct reporter_s reporter_t;

/* Note: This structure must be initialized in external function.
 * This contract needs to be respected otherwise we will have a hard fault by calling NULL functions. */
extern reporter_t reporter_instance;
