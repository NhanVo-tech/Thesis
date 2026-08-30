/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include <logging/log.h>

/* Register the most permissive log level, to allow
 * compile-time filtering based on QLOG_CURRENT_LEVEL locally defined. */
LOG_MODULE_REGISTER(qlog, LOG_LEVEL_DBG);
