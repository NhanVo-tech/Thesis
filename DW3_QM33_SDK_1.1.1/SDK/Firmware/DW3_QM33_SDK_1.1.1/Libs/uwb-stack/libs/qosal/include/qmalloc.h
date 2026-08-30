/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#pragma once

#include <qassert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/** QUOTA_MAGIC - Magic value for quota allocation. */
#define QUOTA_MAGIC 0xDEADDEAD

/** CONFIG_MEM_QUOTA_ID1 - Memory quota ID1, defaulted to 8192 Bytes. */
#ifndef CONFIG_MEM_QUOTA_ID1
#define CONFIG_MEM_QUOTA_ID1 8192
#endif

/** CONFIG_MEM_QUOTA_ID2 - Memory quota ID2, defaulted to 4096 Bytes. */
#ifndef CONFIG_MEM_QUOTA_ID2
#define CONFIG_MEM_QUOTA_ID2 4096
#endif

/** CONFIG_MEM_QUOTA_RANGING_REPORT - Memory quota of the ranging reports, defaulted to use
 * CONFIG_MEM_QUOTA_ID1 pool. */
#ifndef CONFIG_MEM_QUOTA_RANGING_REPORT
#define CONFIG_MEM_QUOTA_RANGING_REPORT MEM_QUOTA_ID1
#endif

/** CONFIG_MEM_QUOTA_PSDU_REPORT - Memory quota of the PSDU reports, defaulted to use
 * CONFIG_MEM_QUOTA_ID1 pool. */
#ifndef CONFIG_MEM_QUOTA_PSDU_REPORT
#define CONFIG_MEM_QUOTA_PSDU_REPORT MEM_QUOTA_ID1
#endif

/** CONFIG_MEM_QUOTA_UCI_REPORT - Memory quota of the UCI reports, defaulted to use
 * CONFIG_MEM_QUOTA_ID2 pool. */
#ifndef CONFIG_MEM_QUOTA_UCI_REPORT
#define CONFIG_MEM_QUOTA_UCI_REPORT MEM_QUOTA_ID2
#endif

/** enum mem_quota_id - Memory quota ID. */
enum mem_quota_id {
	MEM_QUOTA_ID_INFINITE,
#ifdef CONFIG_MEM_QUOTA_ID1
	MEM_QUOTA_ID1,
#endif
#ifdef CONFIG_MEM_QUOTA_ID2
	MEM_QUOTA_ID2,
#endif
#ifdef CONFIG_MEM_QUOTA_ID3
	MEM_QUOTA_ID3,
#endif
#ifdef CONFIG_MEM_QUOTA_ID4
	MEM_QUOTA_ID4,
#endif
	MEM_QUOTA_ID_MAX
};
#if !defined(_GCC_MAX_ALIGN_T) && !defined(__CLANG_MAX_ALIGN_T_DEFINED)
#if defined(__ZEPHYR__)
#include <zephyr/types.h>
typedef z_max_align_t max_align_t;
#else
typedef struct {
	long long __max_align_ll __attribute__((__aligned__(__alignof__(long long))));
	long double __max_align_ld __attribute__((__aligned__(__alignof__(long double))));
} max_align_t;
#endif
#endif /* _GCC_MAX_ALIGN_T && __CLANG_MAX_ALIGN_T_DEFINED */

/** struct quota_alloc_prefix - Header for quota allocation. It will be stored at the beginning of
 * the allocated memory and just before the pointer returned by malloc().
 * @quota_id: Memory quota ID.
 * @size: size of the allocated memory.
 * @magic: Magic value for quota allocation for QUOTA_MAGIC.
 * @align: Padding for alignment.
 */
struct quota_alloc_prefix {
	union {
		struct {
			enum mem_quota_id quota_id;
			uint16_t size;
			uint32_t magic;
		};
		char align[__alignof__(max_align_t)];
	};
};

/** M2Q - Macro to get the header from a pointer. */
#define M2Q(m) (struct quota_alloc_prefix *)((uintptr_t)(m) - sizeof(struct quota_alloc_prefix))
/** Q2M - Macro to get the pointer from a header. */
#define Q2M(q) (void *)((uint8_t *)(q) + sizeof(struct quota_alloc_prefix))

extern uint32_t allocation_quotas[];

/**
 * qmalloc_internal() - Allocate memory.
 * @size: Number of bytes to allocate.
 *
 * Return: Pointer to the allocated memory.
 */
void *qmalloc_internal(size_t size);

/**
 * qrealloc_internal() - Resize the already allocated memory.
 *
 * A size of zero will free the memory.
 * The memory won't be modified if the allocation fails.
 *
 * @ptr: Pointer to previously allocated memory.
 * @new_size: New size for the memory, in bytes.
 *
 * Return: Pointer to the reallocated memory, NULL in case of failure or if new_size is zero.
 */
void *qrealloc_internal(void *ptr, size_t new_size);

/**
 * qfree_internal() - Free memory allocated with qmalloc().
 *
 * @ptr: Pointer to previously allocated memory. NULL will do nothing.
 */
void qfree_internal(void *ptr);

/**
 * qmalloc() - Allocate memory.
 *
 * The internal quota will be set to the infinite quota.
 *
 * @size: Number of bytes to allocate.
 *
 * Return: Pointer to the allocated memory or NULL in case of failure.
 */
void *qmalloc(size_t size);
/**
 * qcalloc() - Allocate memory and set it to 0.
 *
 * The internal quota will be set to the infinite quota.
 *
 * @nb_items: Number of items to allocate.
 * @item_size: Size of an item.
 *
 * Return: Pointer to the allocated memory or NULL in case of failure.
 */
void *qcalloc(size_t nb_items, size_t item_size);

/**
 * qfree() - Free memory allocated with qmalloc().
 *
 * The memory freed must be allocated with qmalloc() or qcalloc() (or the _quota versions) as the
 * quota header is taken into account.
 *
 * @ptr: Pointer to previously allocated memory, NULL will do nothing.
 */
void qfree(void *ptr);

/**
 * qrealloc() - Resize the already allocated memory.
 *
 * A size of zero will free the memory.
 * The memory won't be modified if the allocation fails.
 * The memory reallocated must be allocated with qmalloc() or qcalloc() (or the _quota versions) as
 * the quota header is taken into account.
 *
 * @ptr: Pointer to previously allocated memory.
 * @new_size: New size for the memory, in bytes.
 *
 * Return: Pointer to the reallocated memory, NULL in case of failure or if new_size is zero.
 */

void *qrealloc(void *ptr, size_t new_size);

/**
 * qmalloc_quota() - Allocate memory.
 *
 * The internal quota will be set to qid.
 *
 * @size: Number of bytes to allocate.
 * @qid: Quota to use for this allocation.
 *
 * Return: Pointer to the allocated memory or NULL in case of failure.
 */
void *qmalloc_quota(size_t size, enum mem_quota_id qid);

/**
 * qcalloc_quota() - Allocate memory and set it to 0.
 *
 * The internal quota will be set to qid.
 *
 * @nb_items: Number of items to allocate.
 * @item_size: Size of an item.
 * @qid: Quota to use for this allocation.
 *
 * Return: Pointer to the allocated memory or NULL in case of failure.
 */
void *qcalloc_quota(size_t nb_items, size_t item_size, enum mem_quota_id qid);

#ifdef __cplusplus
}
#endif
