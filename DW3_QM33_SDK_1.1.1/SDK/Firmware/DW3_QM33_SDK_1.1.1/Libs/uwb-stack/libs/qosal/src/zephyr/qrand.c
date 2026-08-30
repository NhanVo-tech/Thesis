/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qrand.h"

#include <random/rand32.h>

void qrand_seed(uint32_t seed)
{
	/* no implementation */
}

uint32_t qrand_rand(void)
{
	return sys_rand32_get();
}
