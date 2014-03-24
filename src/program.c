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
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "error.h"
#include "eval.h"
#include "node.h"
#include "program.h"
#include "register.h"
#include "symbol.h"

#include "grammar.h"

struct prog *grammar_parse_file(const char *filename);

static GSList *files = NULL;
static GSList *macros = NULL;

GSList *prog_ctx_stack = NULL;

static GHashTable *exports = NULL;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct prog *prog_new(enum prog_type type, const char *name) {
	struct prog *new = g_malloc(sizeof(*new));
	new->type = type;
	new->name = g_strdup(name);
	new->lines = NULL;
	new->next_new_line = &new->lines;
	return new;
}

struct prog *prog_new_file(const char *filename) {
	for (GSList *l = files; l; l = l->next) {
		struct prog *f = l->data;
		if (0 == strcmp(filename, f->name)) {
			return f;
		}
	}
	struct prog *file = grammar_parse_file(filename);
	if (!file)
		return NULL;
	files = g_slist_prepend(files, file);
	return file;
}

struct prog *prog_new_macro(const char *name) {
	if (prog_macro_by_name(name)) {
		error(error_type_syntax, "attempt to redefined macro '%s'", name);
		return NULL;
	}
	struct prog *macro = prog_new(prog_type_macro, name);
	macros = g_slist_prepend(macros, macro);
	return macro;
}

void prog_free(struct prog *f) {
	g_slist_free_full(f->lines, (GDestroyNotify)prog_line_free);
	g_free(f->name);
	g_free(f);
}

void prog_free_all(void) {
	g_slist_free_full(macros, (GDestroyNotify)prog_free);
	g_slist_free_full(files, (GDestroyNotify)prog_free);
	prog_free_exports();
}

struct prog *prog_macro_by_name(const char *name) {
	for (GSList *l = macros; l; l = l->next) {
		struct prog *f = l->data;
		if (0 == strcmp(name, f->name)) {
			return f;
		}
	}
	return NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct prog_line *prog_line_new(struct node *label, struct node *opcode, struct node *args) {
	struct prog_line *l;
	l = g_malloc(sizeof(*l));
	l->ref = 1;
	l->label = label;
	l->opcode = opcode;
	l->args = args;
	l->text = NULL;
	return l;
}

void prog_line_free(struct prog_line *line) {
	if (!line)
		return;
	if (line->ref == 0) {
		error_abort("internal: attempt to free prog_line with ref=0");
	}
	line->ref--;
	if (line->ref > 0)
		return;
	node_free(line->label);
	node_free(line->opcode);
	node_free(line->args);
	if (line->text)
		g_free(line->text);
	g_free(line);
}

struct prog_line *prog_line_ref(struct prog_line *line) {
	if (!line)
		return NULL;
	line->ref++;
	return line;
}

void prog_line_set_text(struct prog_line *line, char *text) {
	if (line) {
		line->text = text;
	} else {
		printf("no line...\n");
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

struct prog_ctx *prog_ctx_new(struct prog *prog) {
	struct prog_ctx *new = g_malloc(sizeof(*new));
	new->prog = prog;
	new->line = NULL;
	new->line_number = 0;
	prog_ctx_stack = g_slist_prepend(prog_ctx_stack, new);
	return new;
}

void prog_ctx_free(struct prog_ctx *ctx) {
	/* It doesn't make sense to free a context partway up the stack */
	assert(prog_ctx_stack != NULL);
	assert(prog_ctx_stack->data == ctx);
	prog_ctx_stack = g_slist_remove(prog_ctx_stack, ctx);
	g_free(ctx);
}

void prog_ctx_add_line(struct prog_ctx *ctx, struct prog_line *line) {
	assert(ctx != NULL);
	struct prog *prog = ctx->prog;
	assert(prog != NULL);
	assert(prog->next_new_line != NULL);
	*(prog->next_new_line) = g_slist_append(*prog->next_new_line, line);
	prog->next_new_line = &(*prog->next_new_line)->next;
	ctx->line_number++;
}

struct prog_line *prog_ctx_next_line(struct prog_ctx *ctx) {
	assert(ctx != NULL);
	assert(ctx->prog != NULL);
	if (ctx->line_number == 0) {
		ctx->line = ctx->prog->lines;
		ctx->line_number++;
	} else if (ctx->line) {
		ctx->line = ctx->line->next;
		ctx->line_number++;
	}
	return ctx->line->data;
}

_Bool prog_ctx_end(struct prog_ctx *ctx) {
	assert(ctx != NULL);
	if (!ctx->prog || !ctx->prog->lines)
		return 1;
	if (ctx->line_number == 0 || ctx->line->next)
		return 0;
	return 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
 * Export a macro or symbol.
 */

void prog_export(const char *name) {
	if (!exports) {
		exports = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);
	}
	g_hash_table_add(exports, g_strdup(name));
}

void prog_free_exports(void) {
	if (exports) {
		g_hash_table_destroy(exports);
		exports = NULL;
	}
}

static void print_exported(const char *key, void *value, FILE *f) {
	(void)value;
	struct prog *macro = prog_macro_by_name(key);
	if (macro) {
		fprintf(f, "%s\tmacro\n", key);
		for (GSList *l = macro->lines; l; l = l->next) {
			struct prog_line *line = l->data;
			if (!line)
				continue;
			node_print(f, line->label);
			fprintf(f, "\t");
			node_print(f, line->opcode);
			fprintf(f, "\t");
			node_print_array(f, line->args);
			fprintf(f, "\n");
		}
		fprintf(f, "\tendm\n");
		return;
	}
	struct node *sym = symbol_get(key);
	if (sym) {
		struct node *n = eval_node(sym);
		node_free(sym);
		if (!n)
			return;
		int delim = 0;
		switch (node_type_of(n)) {
		default:
			error(error_type_syntax, "can't export symbol type");
			node_free(n);
			return;
		case node_type_int:
		case node_type_float:
		case node_type_reg:
			break;
		case node_type_string:
			delim = '/';
			break;
		}
		fprintf(f, "%s\tequ\t", key);
		if (delim) fputc(delim, f);
		node_print(f, n);
		if (delim) fputc(delim, f);
		fprintf(f, "\n");
		node_free(n);
	}
}

void prog_print_exports(FILE *f) {
	if (!exports)
		return;
	g_hash_table_foreach(exports, (GHFunc)print_exported, f);
}
