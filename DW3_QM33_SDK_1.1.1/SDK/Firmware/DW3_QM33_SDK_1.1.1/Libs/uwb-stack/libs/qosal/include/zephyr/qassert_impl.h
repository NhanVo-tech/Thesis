/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#ifndef QASSERT
#include <kernel.h>

#define QASSERT(cond)              \
	do {                       \
		if (!(cond)) {     \
			k_panic(); \
		}                  \
	} while (0)
#endif /* QASSERT */
