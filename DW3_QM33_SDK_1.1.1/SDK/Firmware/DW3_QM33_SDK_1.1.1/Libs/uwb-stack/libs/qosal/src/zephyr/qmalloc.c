/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qmalloc.h"

#include <stdlib.h>
#define LOG_TAG ""
#include "qlog.h"

#define QMALLOC_PROFILING(...)                                 \
	do {                                                   \
		if (IS_ENABLED(CONFIG_QOSAL_PROFILING_MALLOC)) \
			QLOGI(__VA_ARGS__);                    \
	} while (0);

uint32_t allocation_quotas[] = {
	~0,
#ifdef CONFIG_MEM_QUOTA_ID1
	CONFIG_MEM_QUOTA_ID1,
#endif
#ifdef CONFIG_MEM_QUOTA_ID2
	CONFIG_MEM_QUOTA_ID2,
#endif
#ifdef CONFIG_MEM_QUOTA_ID3
	CONFIG_MEM_QUOTA_ID3,
#endif
#ifdef CONFIG_MEM_QUOTA_ID4
	CONFIG_MEM_QUOTA_ID4,
#endif
};

void *qmalloc_internal(size_t size)
{
	void *ptr = malloc(size);
	QMALLOC_PROFILING("qmalloc: call %p, p %p, sz %zu", __builtin_return_address(0), ptr, size);
	return ptr;
}

void *qrealloc_internal(void *ptr, size_t new_size)
{
	void *new_ptr = realloc(ptr, new_size);
	QMALLOC_PROFILING("qrealloc: call %p, p %p, np %p, sz %zu", __builtin_return_address(0),
			  ptr, new_ptr, new_size);
	return new_ptr;
}

void qfree_internal(void *ptr)
{
	QMALLOC_PROFILING("qfree: call %p, p %p", __builtin_return_address(0), ptr);
	free(ptr);
}
