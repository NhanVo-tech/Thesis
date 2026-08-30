/*
 * SPDX-FileCopyrightText: Copyright (c) 2024-2025 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#ifndef COMPAT_LINUX_SKBUFF_H
#define COMPAT_LINUX_SKBUFF_H

#include "uwbmac/uwbmac_buf.h"

#include <linux/slab.h>
#include <qassert.h>
#include <qerr.h>
#include <stdbool.h>
#include <stdint.h>

struct sk_buff_head {
	struct uwbmac_buf_queue queue;
};

struct sk_buff {
	union {
		struct uwbmac_buf buf;
		/* Linux is not using typedefs, so no better solution than to
		 * keep both structures synchronized. */
		struct {
			struct uwbmac_buf *next;
			struct uwbmac_buf *prev;
			uint8_t *data;
			unsigned int len;
			uint16_t queue_mapping;
			uint32_t reserved_tailroom;
			uint8_t *head;
			uint8_t *end;
			char cb[UWBMAC_BUF_CB_SIZE] __attribute__((__aligned__(8)));
		};
	};
};

static inline struct sk_buff *qalloc_skb(unsigned int size, int flags, enum mem_quota_id quota_id)
{
	return (struct sk_buff *)uwbmac_buf_alloc_quota(size, quota_id);
}

static inline struct sk_buff *alloc_skb(unsigned int size, int flags)
{
	return (struct sk_buff *)uwbmac_buf_alloc(size);
}

static inline void kfree_skb(struct sk_buff *skb)
{
	if (skb)
		uwbmac_buf_free(&skb->buf);
}

static inline void skb_reserve(struct sk_buff *skb, unsigned int len)
{
	uwbmac_buf_reserve(&skb->buf, len);
}

static inline unsigned int skb_headroom(const struct sk_buff *skb)
{
	return uwbmac_buf_headroom(&skb->buf);
}

static inline int skb_tailroom(const struct sk_buff *skb)
{
	return uwbmac_buf_tailroom(&skb->buf);
}

static inline unsigned char *skb_tail_pointer(struct sk_buff *skb)
{
	return skb->data + skb->len;
}

static inline void skb_trim(struct sk_buff *skb, unsigned int len)
{
	uwbmac_buf_trim(&skb->buf, len);
}

static inline void *skb_put(struct sk_buff *skb, unsigned int len)
{
	return uwbmac_buf_put(&skb->buf, len);
}

static inline void *skb_put_data(struct sk_buff *skb, const void *data, unsigned int len)
{
	uint8_t *tmp = skb->data + skb->len;

	uwbmac_buf_put_data(&skb->buf, data, len);
	return tmp;
}

static inline void skb_put_u8(struct sk_buff *skb, uint8_t v)
{
	uwbmac_buf_put_u8(&skb->buf, v);
}

static inline void *skb_push(struct sk_buff *skb, unsigned int len)
{
	return uwbmac_buf_push(&skb->buf, len);
}

static inline void *skb_pull(struct sk_buff *skb, unsigned int len)
{
	if (len > skb->len)
		return NULL;
	uwbmac_buf_pull(&skb->buf, len);
	return skb->data;
}

static inline void skb_tailroom_reserve(struct sk_buff *skb, unsigned int mtu,
					unsigned int needed_tailroom)
{
	uwbmac_buf_reserve_tailroom(&skb->buf, needed_tailroom);
}

static inline int skb_availroom(const struct sk_buff *skb)
{
	return skb_tailroom(skb);
}

static inline void skb_queue_head_init(struct sk_buff_head *list)
{
	uwbmac_buf_queue_init(&list->queue);
}

static inline int skb_queue_empty(const struct sk_buff_head *list)
{
	return uwbmac_buf_queue_empty(&list->queue);
}

static inline struct sk_buff *skb_peek(struct sk_buff_head *list)
{
	return (struct sk_buff *)uwbmac_buf_queue_peek(&list->queue);
}

static inline void skb_queue_head(struct sk_buff_head *list, struct sk_buff *newsk)
{
	uwbmac_buf_queue_push(&list->queue, &newsk->buf);
}

static inline void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *newsk)
{
	uwbmac_buf_queue_put(&list->queue, &newsk->buf);
}

static inline struct sk_buff *skb_dequeue(struct sk_buff_head *list)
{
	return (struct sk_buff *)uwbmac_buf_queue_pop(&list->queue);
}

static inline void skb_queue_purge(struct sk_buff_head *list)
{
	uwbmac_buf_queue_purge(&list->queue);
}

static inline struct sk_buff *skb_queue_next(const struct sk_buff_head *list,
					     const struct sk_buff *skb)
{
	QASSERT(!uwbmac_buf_queue_is_last(&list->queue, &skb->buf));
	return (struct sk_buff *)uwbmac_buf_queue_next(&list->queue, &skb->buf);
}

static inline bool skb_queue_is_last(const struct sk_buff_head *list, const struct sk_buff *skb)
{
	return uwbmac_buf_queue_is_last(&list->queue, &skb->buf);
}

static inline int skb_zerocopy(struct sk_buff *to, struct sk_buff *from, int len, int hlen)
{
	if (len > skb_availroom(to))
		return QERR_ENOBUFS;

	skb_put_data(to, from->data, len);

	return QERR_SUCCESS;
}

static inline bool skb_is_nonlinear(const struct sk_buff *skb)
{
	return false;
}

static inline unsigned int skb_headlen(const struct sk_buff *skb)
{
	return skb->len;
}

#define skb_walk_frags(skb, iter) for (iter = skb; iter; iter = NULL)

#endif /* COMPAT_LINUX_SKBUFF_H */
