/*

asm6809, a Motorola 6809 cross assembler
Copyright 2013-2015 Ciaran Anscomb

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

*/

#include "config.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xalloc.h"

#include "assemble.h"
#include "dict.h"
#include "error.h"
#include "eval.h"
#include "node.h"
#include "section.h"
#include "slist.h"
#include "symbol.h"

/*
 * When asserted, don't raise an error for undefined symbols.
 */

_Bool symbol_ignore_undefined = 0;

/*
 * Record the pass in which each symbol was entered into the table.  This can
 * be used to detect multiple definitions without cycling through a new table
 * each pass.
 */

struct symbol {
	unsigned pass;
	struct node *node;
};

struct symbol_local {
	unsigned line_number;
	struct node *node;
};

static struct dict *symbols = NULL;

static void symbol_free(struct symbol *s) {
	node_free(s->node);
	free(s);
}

static void init_table(void) {
	symbols = dict_new_full(dict_str_hash, dict_str_equal, free, (Hash_data_freer)symbol_free);
}

void symbol_set(const char *key, struct node *value, _Bool changeable, unsigned pass) {
	_Bool is_inconsistent = symbol_force_set(key, value, changeable, pass);
	if (is_inconsistent && !changeable) {
		error(error_type_inconsistent, "value of '%s' unstable", key);
	}
}

_Bool symbol_force_set(const char *key, struct node *value, _Bool changeable, unsigned pass) {
	if (!symbols)
		init_table();
	struct symbol *olds = dict_lookup(symbols, key);
	if (!changeable && olds && olds->pass == pass) {
		error(error_type_syntax, "symbol '%s' redefined", key);
		return 0;
	}
	struct symbol *news = xmalloc(sizeof(*news));
	news->pass = pass;
	news->node = eval_node(value);
	_Bool is_inconsistent = (olds && !node_equal(olds->node, news->node));
	char *key_copy = xstrdup(key);
	dict_insert(symbols, key_copy, news);
	return is_inconsistent;
}

struct node *symbol_try_get(const char *key) {
	if (!symbols)
		init_table();
	struct symbol *s = dict_lookup(symbols, key);
	if (!s)
		return NULL;
	return node_ref(s->node);
}

struct node *symbol_get(const char *key) {
	struct node *n = symbol_try_get(key);
	if (!n) {
		if (symbol_ignore_undefined) {
			return node_new_int(0);
		} else {
			error(error_type_inconsistent, "symbol '%s' not defined", key);
		}
	}
	return n;
}

void symbol_free_all(void) {
	if (!symbols)
		return;
	dict_destroy(symbols);
	symbols = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void symbol_local_free(struct symbol_local *sym) {
	node_free(sym->node);
	free(sym);
}

static void symbol_local_list_free(struct slist *sym_list) {
	slist_free_full(sym_list, (slist_free_func)symbol_local_free);
}

struct dict *symbol_local_table_new(void) {
	return dict_new_full(dict_direct_hash, dict_direct_equal, NULL, (slist_free_func)symbol_local_list_free);
}

struct node *symbol_local_backref(struct dict *table, long key, unsigned line_number) {
	struct slist *sym_list = dict_lookup(table, (void *)key);
	if (sym_list) {
		struct symbol_local *sym = NULL;
		for (struct slist *l = sym_list; l; l = l->next) {
			struct symbol_local *ls = l->data;
			if (ls->line_number <= line_number &&
			    (!sym || ls->line_number > sym->line_number)) {
				sym = ls;
			}
		}
		if (sym) {
			return node_ref(sym->node);
		}
	}
	error(error_type_inconsistent, "backref '%ld' not defined", key);
	return NULL;
}

struct node *symbol_local_fwdref(struct dict *table, long key, unsigned line_number) {
	struct slist *sym_list = dict_lookup(table, (void *)key);
	if (sym_list) {
		struct symbol_local *sym = NULL;
		for (struct slist *l = sym_list; l; l = l->next) {
			struct symbol_local *ls = l->data;
			if (ls->line_number > line_number &&
			    (!sym || ls->line_number < sym->line_number)) {
				sym = ls;
			}
		}
		if (sym) {
			return node_ref(sym->node);
		}
	}
	error(error_type_inconsistent, "fwdref '%ld' not defined", key);
	return NULL;
}

void symbol_local_set(struct dict *table, long key, unsigned line_number, struct node *value,
		      unsigned pass) {
	struct slist *sym_list = dict_lookup(table, (void *)key);
	dict_steal(table, (void *)key);
	struct symbol_local *sym = NULL;
	for (struct slist *l = sym_list; l; l = l->next) {
		struct symbol_local *ls = l->data;
		if (ls->line_number == line_number) {
			sym = ls;
			break;
		}
	}
	struct node *newn = eval_node(value);
	if (sym) {
		if (!node_equal(sym->node, newn))
			error(error_type_inconsistent,
			      "value of local label '%ld' unstable", key);
		sym_list = slist_remove(sym_list, sym);
		node_free(sym->node);
	} else {
		sym = xmalloc(sizeof(*sym));
		sym->line_number = line_number;
	}
	sym->node = newn;
	sym_list = slist_prepend(sym_list, sym);
	dict_insert(table, (void *)key, sym_list);
}
