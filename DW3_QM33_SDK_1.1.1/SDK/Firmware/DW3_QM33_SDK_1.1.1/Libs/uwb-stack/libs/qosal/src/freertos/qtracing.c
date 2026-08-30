/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qtracing.h"

#include <stddef.h>

qtracing_cb_t tracing_cb = NULL;

enum qerr qtracing_init(void)
{
	return QERR_ENOTSUP;
}
