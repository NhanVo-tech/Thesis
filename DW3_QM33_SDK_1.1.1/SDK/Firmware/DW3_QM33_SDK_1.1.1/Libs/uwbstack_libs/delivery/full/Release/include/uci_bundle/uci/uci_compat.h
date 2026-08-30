/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#ifdef __KERNEL__
#include <linux/string.h>
#include <linux/types.h>
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#endif

#ifdef __KERNEL__
#include <linux/bug.h>

#define UCI_ASSERT(x)                                                                           \
	do {                                                                                    \
		if (WARN(!(x), "### UCI ASSERTION FAILED %s: %s: %d: %s\n", __FILE__, __func__, \
			 __LINE__, #x))                                                         \
			dump_stack();                                                           \
	} while (0)

#else

#include "qassert.h"
#define UCI_ASSERT(x) QASSERT(x)

#include <qmalloc.h>
#endif
