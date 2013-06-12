/*
 * Copyright 2013 Ciaran Anscomb
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
#include <stdio.h>

#include <glib.h>

#include "assemble.h"
#include "error.h"
#include "eval.h"
#include "node.h"
#include "section.h"
#include "symbol.h"

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
	unsigned pass;
	unsigned line_number;
	struct node *node;
};

static GHashTable *symbols = NULL;

static void symbol_free(struct symbol *s) {
	node_free(s->node);
	g_free(s);
}

static void init_table(void) {
	symbols = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)symbol_free);
}

void symbol_set(const char *key, struct node *value, unsigned pass) {
	if (!symbols)
		init_table();
	struct symbol *olds = g_hash_table_lookup(symbols, key);
	if (olds && olds->pass == pass) {
		error(error_type_syntax, "symbol '%s' redefined", key);
		return;
	}
	struct symbol *news = g_malloc(sizeof(*news));
	news->pass = pass;
	news->node = eval_node(value);
	if (olds && !node_equal(olds->node, news->node))
		error(error_type_inconsistent, "value of '%s' unstable", key);
	char *key_copy = g_strdup(key);
	g_hash_table_insert(symbols, key_copy, news);
}

struct node *symbol_get(const char *key) {
	if (!symbols)
		init_table();
	struct symbol *s = g_hash_table_lookup(symbols, key);
	if (!s) {
		error(error_type_inconsistent, "symbol '%s' not defined", key);
		return NULL;
	}
	return node_ref(s->node);
}

void symbol_free_all(void) {
	if (!symbols)
		return;
	g_hash_table_destroy(symbols);
	symbols = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void symbol_local_free(struct symbol_local *sym) {
	node_free(sym->node);
	g_free(sym);
}

static void symbol_local_list_free(GSList *sym_list) {
	g_slist_free_full(sym_list, (GDestroyNotify)symbol_local_free);
}

GHashTable *symbol_local_table_new(void) {
	return g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, (GDestroyNotify)symbol_local_list_free);
}

struct node *symbol_local_backref(GHashTable *table, long key, unsigned line_number) {
	GSList *sym_list = g_hash_table_lookup(table, GINT_TO_POINTER(key));
	if (sym_list) {
		struct symbol_local *sym = NULL;
		for (GSList *l = sym_list; l; l = l->next) {
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

struct node *symbol_local_fwdref(GHashTable *table, long key, unsigned line_number) {
	GSList *sym_list = g_hash_table_lookup(table, GINT_TO_POINTER(key));
	if (sym_list) {
		struct symbol_local *sym = NULL;
		for (GSList *l = sym_list; l; l = l->next) {
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

void symbol_local_set(GHashTable *table, long key, unsigned line_number, struct node *value,
		      unsigned pass) {
	GSList *sym_list = g_hash_table_lookup(table, GINT_TO_POINTER(key));
	g_hash_table_steal(table, GINT_TO_POINTER(key));
	struct symbol_local *sym = NULL;
	for (GSList *l = sym_list; l; l = l->next) {
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
		sym_list = g_slist_remove(sym_list, sym);
		node_free(sym->node);
	} else {
		sym = g_malloc(sizeof(*sym));
		sym->line_number = line_number;
	}
	sym->node = newn;
	sym->pass = pass;
	sym_list = g_slist_prepend(sym_list, sym);
	g_hash_table_insert(table, GINT_TO_POINTER(key), sym_list);
}
