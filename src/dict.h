/*
 * asm6809 - a 6809 assembler
 * Copyright 2013-2014 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_DICT_H_
#define ASM6809_DICT_H_

#include <stdbool.h>

#include "hash.h"
#include "hash-pjw.h"

struct dict;

typedef void (*dict_iter_func)(void *k, void *v, void *data);

struct dict *dict_new(Hash_hasher hash_func,
		     Hash_comparator key_equal_func);

struct dict *dict_new_full(Hash_hasher hash_func,
			  Hash_comparator key_equal_func,
			  Hash_data_freer key_destroy_func,
			  Hash_data_freer value_destroy_func);

void dict_destroy(struct dict *);

void *dict_lookup(struct dict *, const void *k);
void dict_insert(struct dict *, const void *k, const void *v);
void dict_replace(struct dict *d, const void *k, const void *v);
void dict_add(struct dict *d, const void *k);
bool dict_remove(struct dict *, const void *k);
bool dict_steal(struct dict *, const void *k);
void dict_foreach(struct dict *, dict_iter_func, void *);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

size_t dict_direct_hash(const void *, size_t);
bool dict_direct_equal(const void *k1, const void *k2);

#define dict_str_hash (hash_pjw)
bool dict_str_equal(const void *k1, const void *k2);

#endif  /* ASM6809_DICT_H_ */
