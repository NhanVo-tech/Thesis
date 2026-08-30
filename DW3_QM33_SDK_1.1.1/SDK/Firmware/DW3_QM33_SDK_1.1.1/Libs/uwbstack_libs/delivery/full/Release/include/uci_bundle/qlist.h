/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "qtils.h"
#include "qtoolchain.h" /* IWYU pragma: keep */

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ARRAY_SIZE - get the number of elements in array @arr
 * (needed for list.h replacement on linux build)
 *
 * @arr: array to be sized
 */
#if !defined(ARRAY_SIZE) && defined(__linux__)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

struct qlist_head {
	struct qlist_head *next;
	struct qlist_head *prev;
};

#define QLIST_HEAD(name) struct qlist_head name = { &(name), &(name) }
#define QLIST_INIT_HEAD qlist_init_head

/**
 * qlist_entry - Get the struct of this entry
 * @ptr: The qlist_head entry pointer.
 * @type: The type of the struct which embed the qlist entry.
 * @member: The name of the qlist entry within the struct.
 */
#define qlist_entry(ptr, type, member) qparent_of(ptr, type, member)

/**
 * qlist_first_entry - Get the struct of the first entry in the qlist.
 * @head: The qlist head pointer.
 * @type: The type of the struct which embed the qlist entry.
 * @member: The name of the qlist entry within the struct.
 */
#define qlist_first_entry(head, type, member) qlist_entry((head)->next, type, member)

/**
 * qlist_last_entry - Get the struct of the last entry in the qlist.
 * @head: The qlist head pointer.
 * @type: The type of the struct which embed the qlist entry.
 * @member: The name of the qlist entry within the struct.
 */
#define qlist_last_entry(head, type, member) qlist_entry((head)->prev, type, member)

/**
 * qlist_next_entry - Get the struct of the next entry in the qlist.
 * @pos: The qlist entry pointer.
 * @member: The name of the qlist entry within the struct.
 */
#define qlist_next_entry(pos, member) qlist_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * qlist_prev_entry - Get the struct of the previous entry in the qlist.
 * @pos: The qlist entry pointer.
 * @member: The name of the qlist entry within the struct.
 */
#define qlist_prev_entry(pos, member) qlist_entry((pos)->member.prev, typeof(*(pos)), member)

static inline void qlist_init_head(struct qlist_head *list)
{
	list->next = list->prev = list;
}

static inline bool qlist_empty(const struct qlist_head *head)
{
	return head->next == head;
}

void qlist_add(struct qlist_head *entry, struct qlist_head *list);
void qlist_add_tail(struct qlist_head *entry, struct qlist_head *list);
void qlist_del(struct qlist_head *entry);
void qlist_move(struct qlist_head *entry, struct qlist_head *list);
void qlist_move_tail(struct qlist_head *entry, struct qlist_head *list);
void qlist_replace(struct qlist_head *entry_old, struct qlist_head *entry_new);
int qlist_is_singular(const struct qlist_head *head);

#define qlist_for_each_entry(pos, head, member)                                           \
	for (pos = qlist_first_entry(head, typeof(*pos), member); &pos->member != (head); \
	     pos = qlist_next_entry(pos, member))

#define qlist_for_each_entry_safe(pos, tmp, head, member)         \
	for (pos = qlist_first_entry(head, typeof(*pos), member), \
	    tmp = qlist_next_entry(pos, member);                  \
	     &pos->member != (head); pos = tmp, tmp = qlist_next_entry(pos, member))

static inline int qlist_is_last(const struct qlist_head *list, const struct qlist_head *head)
{
	return list->next == head;
}

#ifdef __cplusplus
}
#endif
