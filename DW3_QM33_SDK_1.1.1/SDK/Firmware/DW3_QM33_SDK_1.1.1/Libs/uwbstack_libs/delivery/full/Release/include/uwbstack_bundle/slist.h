/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Qorvo US, Inc.
 *
 * SPDX-License-Identifier: LicenseRef-QORVO-1
 */

#pragma once

#include "qtils.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * struct slist_head - Head of the slist.
 */
struct slist_head {
	/** @first: Front of the slist_head. */
	struct slist_node *first;
	/** @last: Back of the slist_head. */
	struct slist_node *last;
};

/**
 * struct slist_node - Node in the slist.
 */
struct slist_node {
	/** @next: Next node in the slist (NULL on the last). */
	struct slist_node *next;
};

#define SLIST_HEAD_INIT()   \
	{                   \
		NULL, NULL, \
	}

#define SLIST_HEAD(name) struct slist_head name = SLIST_HEAD_INIT()

/**
 * init_slist_head - Initializes slist head
 * @head: An slist head.
 */
static inline void init_slist_head(struct slist_head *head)
{
	*head = (struct slist_head)SLIST_HEAD_INIT();
}

/**
 * slist_entry - Get the struct of this entry
 * @ptr: The slist_node pointer.
 * @type: The type of the struct which embed the slist_node.
 * @member: The name of the slist_node within the struct.
 */
#define slist_entry(ptr, type, member) qparent_of(ptr, type, member)

/**
 * slist_first_entry - Get the struct of the first entry in the slist.
 * @head: The slist_head pointer.
 * @type: The type of the struct which embed the slist_node.
 * @member: The name of the slist_node within the struct.
 */
#define slist_first_entry(head, type, member) \
	(slist_empty(head) ? NULL : slist_entry((head)->first, type, member))

/**
 * slist_last_entry - Get the struct of the last entry in the slist.
 * @head: The slist_head pointer.
 * @type: The type of the struct which embed the slist_node.
 * @member: The name of the slist_node within the struct.
 */
#define slist_last_entry(head, type, member) \
	(slist_empty(head) ? NULL : slist_entry((head)->last, type, member))

/**
 * slist_next_entry - Get the next node entry.
 * @pos: The slist_node pointer.
 * @member: The name of the slist_node within the struct.
 */
#define slist_next_entry(pos, member) \
	((pos)->member.next ? slist_entry((pos)->member.next, typeof(*(pos)), member) : NULL)

/**
 * slist_for_each_entry - Iterate over entries of slist of given type
 * @pos: The pointer to use as a iterator.
 * @head: The slist to iterate.
 * @member: The name of the slist_node with the struct.
 */
#define slist_for_each_entry(pos, head, member)                                      \
	for ((pos) = slist_first_entry(head, typeof(*(pos)), member); (pos) != NULL; \
	     (pos) = slist_next_entry(pos, member))

/**
 * slist_for_each_entry_safe - Safe iteration over entries of the slist.
 * @pos: The pointer to use as a iterator.
 * @tmp: Another iterator used as temporary storage
 * @head: The slist to iterate.
 * @member: The name of the slist_node with the struct.
 */
#define slist_for_each_entry_safe(pos, tmp, head, member)             \
	for ((pos) = slist_first_entry(head, typeof(*(pos)), member), \
	    (tmp) = (pos) ? slist_next_entry(pos, member) : NULL;     \
	     (pos) != NULL; (pos) = (tmp), (tmp) = (tmp) ? slist_next_entry(tmp, member) : NULL)

/**
 * slist_empty - tests whether a slist is empty
 * @head: The head of the slist.
 *
 * Return: True if list is empty, else False.
 */
static inline bool slist_empty(const struct slist_head *head)
{
	return head->first == NULL;
}

/**
 * slist_push_front - Pushes a new entry to the slist_head front.
 * @entry: The new entry to add.
 * @head: The head of the slist.
 *
 * Context: Use this function when you want a LIFO list (Last In First Out).
 *          Because "list for each" or "pop" functions are available only on the
 *          front of the slist.
 */
void slist_push_front(struct slist_node *entry, struct slist_head *head);

/**
 * slist_push_back - Pushes a new entry to the slist_head tail.
 * @entry: The new entry to add.
 * @head: The head of the slist.
 *
 * Context: Use this function when you want a FIFO list (First In First Out).
 *          Because "list for each" or "pop" functions are available only on the
 *          front of the slist.
 */
void slist_push_back(struct slist_node *entry, struct slist_head *head);

/**
 * slist_pop_front - Pop an entry from the slist_head front.
 * @head: The head of the slist.
 *
 * Return: The entry removed from the slist or NULL if empty.
 */
struct slist_node *slist_pop_front(struct slist_head *head);

#ifdef __cplusplus
}
#endif
