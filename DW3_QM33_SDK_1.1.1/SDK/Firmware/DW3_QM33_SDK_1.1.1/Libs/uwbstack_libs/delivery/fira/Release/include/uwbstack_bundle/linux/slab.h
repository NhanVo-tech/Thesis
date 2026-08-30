/*
 * SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#ifndef COMPAT_LINUX_SLAB_H
#define COMPAT_LINUX_SLAB_H

#include "qmalloc.h"

#include <stdint.h>
#include <string.h>

/**
 * kmemzero_explicit() - clear memory.
 * @s: Pointer to memory to clear
 * @count: number of bytes to clear
 */
static inline void kmemzero_explicit(void *s, size_t count)
{
	memset(s, 0, count);
	__asm__ __volatile__("" : : "r"(s) : "memory");
}

/* NOTE: This assumes the size of the allocation is the size of the pointee
 * type. This is currently always the case in the UWB stack but nothing
 * enforces this. */
#define kfree_sensitive(arg)                                      \
	do {                                                      \
		if (arg) {                                        \
			kmemzero_explicit((arg), sizeof(*(arg))); \
			qfree((arg));                             \
		}                                                 \
	} while (0)

enum gfp_e {
	GFP_KERNEL,
};

static inline void *kmalloc(size_t size, __attribute__((unused)) int flags)
{
	return qmalloc(size);
}

static inline void *kzalloc(size_t size, __attribute__((unused)) int flags)
{
	return qcalloc(1, size);
}

static inline void kfree(void *ptr)
{
	qfree(ptr);
}

static inline void *krealloc(void *ptr, size_t size, __attribute__((unused)) int flags)
{
	return qrealloc(ptr, size);
}

#endif /* COMPAT_LINUX_SLAB_H */
