/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qmalloc.h"

#include "FreeRTOS.h"

#include <string.h>

/**
 * struct block_link - Linked list.
 * @px_next_free_block: The next free block in the list.
 * @block_size: The size of the free block.
 *
 * Note: That list is used to free blocks in order
 * of their memory address.
 * This structure came from freeRTOS sources.
 */
struct block_link {
	struct block_link *px_next_free_block;
	size_t block_size;
};

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
	return pvPortMalloc(size);
}

void qfree_internal(void *ptr)
{
	vPortFree(ptr);
}

void *qrealloc_internal(void *ptr, size_t new_size)
{
	/* Return address of realoc. */
	void *ret_ptr = NULL;
	struct block_link *px_link;
	size_t len;

	/* Get the address of the original pointer. */
	unsigned char *p_original = (unsigned char *)ptr;

	/* Check parameters. */
	if ((ptr == NULL) && (new_size > 0))
		return qmalloc_internal(new_size);

	if ((ptr != NULL) && (new_size > 0)) {
		/* The size of the structure placed at the beginning of each
		allocated memory block must be correctly byte aligned. */
		const size_t heap_struct_size =
			(sizeof(struct block_link) + ((size_t)(portBYTE_ALIGNMENT - 1))) &
			~((size_t)portBYTE_ALIGNMENT_MASK);

		/* The size of every block is stored in a block_link structure,
		 * in the allocated block itself. */
		p_original -= heap_struct_size;
		px_link = (void *)p_original;

		/* Allocate the new pointer. */
		ret_ptr = qmalloc_internal(new_size);
		if (ret_ptr) {
			/* Get the size of the original pointer. */
			len = px_link->block_size - heap_struct_size;

			if (len > new_size)
				len = new_size;

			memcpy(ret_ptr, ptr, len);
			qfree_internal(ptr);
		} else {
			/* Allocation failed; do not modify the original. */
		}
	}

	return ret_ptr;
}
