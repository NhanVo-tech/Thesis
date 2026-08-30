/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#if defined(__GNUC__)
/* GCC-specific code here. */

/**
 * QFFS() - See __builtin_ffs in GCC documentation.
 * @x: The value to test.
 * Returns: one plus the index of the least significant 1-bit of x, or if x is zero, returns zero.
 */
#define QFFS(x) __builtin_ffs(x)

/**
 * LIKELY() - See __builtin_expect in GCC documentation.
 * Help the compiler to understand the condition is likely to be true to optimize the branch.
 * @x: The value to test.
 * Returns: 0 if x is false, 1 if x is true.
 */
#define LIKELY(x) __builtin_expect(!!(x), 1)

/**
 * UNLIKELY() - See __builtin_expect in GCC documentation.
 * Help the compiler to understand the condition is unlikely to be true to optimize the branch.
 * @x: The value to test.
 * Returns: 0 if x is false, 1 if x is true.
 */
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#ifndef __cplusplus
#if __STDC__ && __STDC_VERSION__ < 202311L
#ifndef typeof
/**
 * typeof() - See __builtin_types_compatible_p in GCC documentation.
 * @x: The value to test.
 * Returns: the type of x.
 */
#define typeof(x) __typeof__(x) /* typeof was introduced in C23. */
#endif
#endif
#endif

#endif

#if defined(_IAR_SYSTEMS_ICC__)
/* IAR-specific code here. */
#endif

#if defined(__KEIL__) || defined(__C51__)
/* KEIL-specific code here. */
#endif
