/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-Qorvo-2
 */

#include "qmalloc.h"

void *qmalloc(size_t size)
{
	struct quota_alloc_prefix *q = NULL;
	void *res = NULL;
	if (!size)
		return res;

	q = (struct quota_alloc_prefix *)qmalloc_internal(size + sizeof(struct quota_alloc_prefix));
	if (q) {
		q->size = size;
		q->quota_id = MEM_QUOTA_ID_INFINITE;
		q->magic = QUOTA_MAGIC;
		allocation_quotas[MEM_QUOTA_ID_INFINITE] -= size;
		res = Q2M(q);
	}
	return res;
}

void *qcalloc(size_t nb_items, size_t item_size)
{
	struct quota_alloc_prefix *q = NULL;
	size_t size = nb_items * item_size;
	void *res = NULL;
	if (!size)
		return res;

	q = (struct quota_alloc_prefix *)qmalloc_internal(size + sizeof(struct quota_alloc_prefix));
	if (q) {
		res = Q2M(q);
		memset(res, 0, size);
		q->size = size;
		q->quota_id = MEM_QUOTA_ID_INFINITE;
		q->magic = QUOTA_MAGIC;
		allocation_quotas[MEM_QUOTA_ID_INFINITE] -= size;
	}
	return res;
}

void qfree(void *ptr)
{
	struct quota_alloc_prefix *q = ptr ? M2Q(ptr) : NULL;
	if (q) {
		enum mem_quota_id quota_id = q->quota_id;
		QASSERT(q->magic == QUOTA_MAGIC);
		QASSERT(quota_id < MEM_QUOTA_ID_MAX);
		allocation_quotas[quota_id] += q->size;
	}
	qfree_internal(q);
}

void *qrealloc(void *ptr, size_t new_size)
{
	struct quota_alloc_prefix *q = ptr ? M2Q(ptr) : NULL, *newq = NULL;
	uint16_t old_size = ptr ? q->size : 0;
	enum mem_quota_id quota_id = q ? q->quota_id : MEM_QUOTA_ID_INFINITE;
	void *res = NULL;
	int sdiff = new_size - old_size;

	if (new_size == 0) {
		/* Free old alloc. */
		qfree(ptr);
		return res;
	}

	QASSERT(quota_id < MEM_QUOTA_ID_MAX);
	QASSERT(!q || (q->magic == QUOTA_MAGIC));
	if ((sdiff < 0) || (allocation_quotas[quota_id] >= (size_t)sdiff)) {
		newq = (struct quota_alloc_prefix *)qrealloc_internal(
			q, new_size + sizeof(struct quota_alloc_prefix));
		if (newq) {
			newq->size = new_size;
			newq->quota_id = quota_id;
			newq->magic = QUOTA_MAGIC;
			allocation_quotas[quota_id] -= sdiff;
			res = Q2M(newq);
		}
	}
	return res;
}

void *qmalloc_quota(size_t size, enum mem_quota_id qid)
{
	struct quota_alloc_prefix *q = NULL;
	void *res = NULL;
	if (!size)
		return res;

	QASSERT(qid < MEM_QUOTA_ID_MAX);
	if (allocation_quotas[qid] >= size) {
		q = (struct quota_alloc_prefix *)qmalloc_internal(
			size + sizeof(struct quota_alloc_prefix));
		if (q) {
			q->size = size;
			q->quota_id = qid;
			q->magic = QUOTA_MAGIC;
			allocation_quotas[qid] -= size;
			res = Q2M(q);
		}
	}
	return res;
}

void *qcalloc_quota(size_t nb_items, size_t item_size, enum mem_quota_id qid)
{
	struct quota_alloc_prefix *q = NULL;
	size_t size = nb_items * item_size;
	void *res = NULL;
	if (!size)
		return res;

	QASSERT(qid < MEM_QUOTA_ID_MAX);
	if (allocation_quotas[qid] >= size) {
		q = (struct quota_alloc_prefix *)qmalloc_internal(
			size + sizeof(struct quota_alloc_prefix));
		if (q) {
			memset(Q2M(q), 0, size);
			q->size = size;
			q->quota_id = qid;
			q->magic = QUOTA_MAGIC;
			allocation_quotas[qid] -= size;
			res = Q2M(q);
		}
	}
	return res;
}
