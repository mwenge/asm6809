/*
 * asm6809 - a 6809 assembler
 * Copyright 2013-2014 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_SLIST_H_
#define ASM6809_SLIST_H_

typedef void (*slist_free_func)(void *);
typedef void *(*slist_copy_func)(const void *, void *);
typedef int (*slist_cmp_func)(const void *, const void *);

struct slist {
	struct slist *next;
	void *data;
};

/* Each of these return the new pointer to the head of the list: */
struct slist *slist_append(struct slist *, void *);
struct slist *slist_prepend(struct slist *, void *);
struct slist *slist_insert_before(struct slist *, struct slist *, void *);
struct slist *slist_remove(struct slist *, void *);

void slist_free(struct slist *);
void slist_free_full(struct slist *, slist_free_func);
void slist_free_1(struct slist *);

struct slist *slist_copy(struct slist *);
struct slist *slist_copy_deep(struct slist *, slist_copy_func, void *);

struct slist *slist_sort(struct slist *, slist_cmp_func);
struct slist *slist_concat(struct slist *, struct slist *);

struct slist *slist_find(struct slist *, void *);

#endif  /* ASM6809_SLIST_H_ */
