/*
 * Copyright 2013-2014 Ciaran Anscomb
 *
 * This file is part of asm6809.
 *
 * asm6809 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 *
 * asm6809 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with asm6809.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>

#include "xalloc.h"

#include "slist.h"

/* Wrap data in a new list container */
static struct slist *slist_new(void *data) {
	struct slist *new = malloc(sizeof(*new));
	if (!new) return NULL;
	new->next = NULL;
	new->data = data;
	return new;
}

/* Add new data to tail of list */
struct slist *slist_append(struct slist *list, void *data) {
	return slist_insert_before(list, NULL, data);
}

/* Add new data to head of list */
struct slist *slist_prepend(struct slist *list, void *data) {
	return slist_insert_before(list, list, data);
}

/* Insert new data before given position */
struct slist *slist_insert_before(struct slist *list, struct slist *before, void *data) {
	struct slist *ent = slist_new(data);
	struct slist *iter;
	if (!ent) return list;
	if (!list) return ent;
	ent->next = before;
	if (before == list) return ent;
	for (iter = list; iter; iter = iter->next) {
		if (!iter->next || iter->next == before) {
			iter->next = ent;
			break;
		}
	}
	return list;
}

/* Remove list entry containing data. */
struct slist *slist_remove(struct slist *list, void *data) {
	struct slist **entp;
	if (!data) return list;
	for (entp = &list; *entp; entp = &(*entp)->next) {
		if ((*entp)->data == data) break;
	}
	if (*entp) {
		struct slist *ent = *entp;
		*entp = ent->next;
		free(ent);
	}
	return list;
}

/* Free entire list. */
void slist_free(struct slist *list) {
	slist_free_full(list, NULL);
}

/* Free entire list, call free_func on all data. */
void slist_free_full(struct slist *list, slist_free_func free_func) {
	while (list) {
		struct slist *next = list->next;
		if (free_func && list->data)
			free_func(list->data);
		free(list);
		list = next;
	}
}

void slist_free_1(struct slist *list) {
	free(list);
}

/* Copy list structure. */
struct slist *slist_copy(struct slist *list) {
	struct slist *new = NULL;
	struct slist **entp = &new;
	for (; list; list = list->next) {
		*entp = slist_new(list->data);
		entp = &(*entp)->next;
	}
	return new;
}

/* Copy list, calling an allocator for referenced data. */
struct slist *slist_copy_deep(struct slist *list, slist_copy_func copy_func, void *copy_data) {
	struct slist *new = slist_copy(list);
	for (struct slist *l = new; l; l = l->next) {
		l->data = copy_func(l->data, copy_data);
	}
	return new;
}

static struct slist *slist_merge(struct slist *left, struct slist *right, slist_cmp_func cmp_func) {
	struct slist *new = NULL;
	struct slist **newp = &new;
	while (left || right) {
		if (left && right) {
			if (cmp_func(left->data, right->data) <= 0) {
				*newp = left;
				left = left->next;
			} else {
				*newp = right;
				right = right->next;
			}
		} else if (left) {
			*newp = left;
			left = left->next;
		} else {
			*newp = right;
			right = right->next;
		}
		newp = &(*newp)->next;
	}
	*newp = NULL;
	return new;
}

struct slist *slist_sort(struct slist *list, slist_cmp_func cmp_func) {
	if (!list || !list->next)
		return list;
	struct slist *left = NULL;
	struct slist **leftp = &left;
	struct slist *right = NULL;
	struct slist **rightp = &right;
	while (list) {
		*leftp = list;
		list = list->next;
		leftp = &(*leftp)->next;
		if (list) {
			*rightp = list;
			list = list->next;
			rightp = &(*rightp)->next;
		}
	}
	*leftp = *rightp = NULL;
	left = slist_sort(left, cmp_func);
	right = slist_sort(right, cmp_func);
	return slist_merge(left, right, cmp_func);
}

struct slist *slist_concat(struct slist *list1, struct slist *list2) {
	if (!list2)
		return list1;
	if (!list1)
		return list2;
	while (list1->next)
		list1 = list1->next;
	list1->next = list2;
	return list1;
}

/* Find list entry containing data */
struct slist *slist_find(struct slist *list, void *data) {
	struct slist *ent;
	for (ent = list; ent; ent = ent->next) {
		if (ent->data == data)
			return ent;
	}
	return NULL;
}
