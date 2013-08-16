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
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "error.h"
#include "opcode.h"
#include "section.h"
#include "symbol.h"

static GHashTable *sections = NULL;
static unsigned span_sequence = 0;

struct section *cur_section = NULL;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static struct section_span *sect_span_new(void) {
	assert(cur_section != NULL);
	struct section_span *new = g_malloc(sizeof(*new));
	new->ref = 1;
	new->sequence = span_sequence++;
	new->org = 0;
	new->put = 0;
	new->size = 0;
	new->allocated = 0;
	new->data = NULL;
	return new;
}

static struct section_span *sect_span_ref(struct section_span *span) {
	if (!span)
		return NULL;
	span->ref++;
	return span;
}

static void sect_span_free(struct section_span *span) {
	if (!span)
		return;
	if (span->ref == 0) {
		error_abort("internal: attempt to free sect_span with ref=0");
	}
	span->ref--;
	if (span->ref > 0)
		return;
	if (span->data)
		g_free(span->data);
	g_free(span);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static struct section *section_new(void) {
	struct section *sect = g_malloc(sizeof(*sect));
	sect->spans = NULL;
	sect->span = NULL;
	sect->local_labels = symbol_local_table_new();
	sect->pass = (unsigned)-1;
	sect->line_number = 0;
	sect->pc = 0;
	sect->dp = -1;
	sect->last_pc = 0;
	return sect;
}

void section_free(struct section *sect) {
	if (!sect)
		return;
	g_hash_table_destroy(sect->local_labels);
	g_slist_free_full(sect->spans, (GDestroyNotify)sect_span_free);
	g_free(sect);
}

void section_free_all(void) {
	if (sections)
		g_hash_table_destroy(sections);
	sections = NULL;
	cur_section = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void section_set(const char *name, unsigned pass) {
	if (!sections)
		sections = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, (GDestroyNotify)section_free);

	struct section *next_section = g_hash_table_lookup(sections, name);
	if (!next_section) {
		next_section = section_new();
		char *key = g_strdup(name);
		g_hash_table_insert(sections, key, next_section);
	}

	if (next_section->pass != pass) {
		if (next_section->spans) {
			g_slist_free_full(next_section->spans, (GDestroyNotify)sect_span_free);
			next_section->spans = NULL;
			next_section->span = NULL;
		}
		if (cur_section && cur_section->pass == pass) {
			next_section->pc = cur_section->last_pc;
		} else {
			next_section->pc = 0;
		}
		next_section->pass = pass;
		next_section->dp = -1;
		next_section->line_number = 0;
	}

	cur_section = next_section;
	return;
}

static void verify_section(gpointer key, gpointer value, gpointer user_data) {
	struct section *sect = value;
	unsigned pass = GPOINTER_TO_INT(user_data);
	if (sect) {
		if (pass == 0 || sect->pass != pass) {
			if (sect->last_pc != sect->pc) {
				sect->last_pc = sect->pc;
				error(error_type_inconsistent, NULL);
			}
		}
	}
}

void section_finish_pass(unsigned pass) {
	g_hash_table_foreach(sections, verify_section, GINT_TO_POINTER(pass + 1));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static int span_cmp(struct section_span *a, struct section_span *b) {
	if (a->put < b->put) return -1;
	if (a->put > b->put) return 1;
	if (a->sequence < b->sequence) return -1;
	return 1;
}

/* TODO: When merging spans this should create a new span and free the old
 * ones.  At the moment it breaks reference counting... */

void section_coalesce(struct section *sect, _Bool sort, _Bool pad) {

	GSList *spans = sect->spans;
	if (sort)
		spans = g_slist_sort(spans, (GCompareFunc)span_cmp);
	for (GSList *l = spans; l && l->next; l = l->next) {
		do {
			GSList *ln = l->next;
			struct section_span *span = l->data;
			struct section_span *nspan = ln->data;
			unsigned span_end = span->put + span->size;
			printf("put=%04x size=%04x    next put=%04x size=%04x\n", span->put, span->size, nspan->put, nspan->size);

			if (span_end > nspan->put) {
				error(error_type_data, "data at $%04X overlaps data at $%04X", span->put, nspan->put);
				// truncate earlier span
				span->size -= (span_end - nspan->put);
			} else if (pad && span_end < nspan->put) {
				unsigned npad = nspan->put - span_end;
				if ((span->size + npad) > span->allocated) {
					span->allocated = span->size + npad;
					span->data = g_realloc(span->data, span->allocated);
				}
				memset(span->data + span->size, 0, npad);
				span->size = span->size + npad;
				span_end = span->put + span->size;
			}
			if (span_end == nspan->put) {
				if ((span->size + nspan->size) > span->allocated) {
					span->allocated = span->size + nspan->size;
					span->data = g_realloc(span->data, span->allocated);
				}
				memcpy(span->data + span->size, nspan->data, nspan->size);
				span->size += nspan->size;
				l->next = l->next->next;
				sect_span_free(nspan);
				g_slist_free_1(ln);
				ln = l->next;
				span = l->data;
				nspan = ln->data;
				span_end = span->put + span->size;
			} else {
				break;
			}
		} while (l->next);
	}

	sect->spans = spans;
}

struct section *section_coalesce_all(_Bool pad) {
	struct section *sect = section_new();

	GList *sect_list = g_hash_table_get_values(sections);
	for (GList *l = sect_list; l; l = l->next) {
		struct section *s = l->data;
		sect->spans = g_slist_concat(sect->spans, g_slist_copy_deep(s->spans, (GCopyFunc)sect_span_ref, NULL));
	}
	g_list_free(sect_list);

	section_coalesce(sect, 1, pad);
	return sect;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
 * Emit data.  Adds bytes to the current span, or creates a new span if
 * appropriate.
 */

static int op_size(unsigned short op) {
	return (op & 0xff00) ? 2 : 1;
}

#define next_put(s) ((s)->put + (s)->size)
#define next_pc(s) ((int)((s)->org + (s)->size))

void sect_emit(enum sect_emit_type type, ...) {
	assert(cur_section != NULL);
	struct section_span *span = cur_section->span;

	if (!span || (cur_section->put != next_put(span)) ||
	    (cur_section->pc != next_pc(span))) {
		if (!span || span->size != 0) {
			span = sect_span_new();
			cur_section->spans = g_slist_append(cur_section->spans, span);
		}
		span->put = cur_section->put;
		span->org = cur_section->pc;
	}
	cur_section->span = span;

	va_list ap;
	va_start(ap, type);
	struct opcode *op;
	int output = 0;
	int nbytes;
	_Bool pad = 0;

	switch (type) {
	case sect_emit_type_pad:
		nbytes = va_arg(ap, int);
		pad = 1;
		break;
	case sect_emit_type_op_immediate:
		op = va_arg(ap, struct opcode *);
		output = op->immediate;
		nbytes = op_size(output);
		break;
	case sect_emit_type_op_direct:
		op = va_arg(ap, struct opcode *);
		output = op->direct;
		nbytes = op_size(output);
		break;
	case sect_emit_type_op_indexed:
		op = va_arg(ap, struct opcode *);
		output = op->indexed;
		nbytes = op_size(output);
		break;
	case sect_emit_type_op_extended:
		op = va_arg(ap, struct opcode *);
		output = op->extended;
		nbytes = op_size(output);
		break;
	case sect_emit_type_imm8:
	case sect_emit_type_rel8:
		output = va_arg(ap, int);
		nbytes = 1;
		break;
	case sect_emit_type_imm16:
	case sect_emit_type_rel16:
		output = va_arg(ap, int);
		nbytes = 2;
		break;
	default:
		error(error_type_fatal, "unknown emit format");
		return;
	}

	if (cur_section->pc < 0) {
		error(error_type_out_of_range, "assembling to negative address");
	}

	cur_section->put += nbytes;
	cur_section->pc += nbytes;
	if (type == sect_emit_type_rel8 || type == sect_emit_type_rel16)
		output -= cur_section->pc;
	if (type == sect_emit_type_rel8) {
		if (output < -128 || output > 127)
			error(error_type_out_of_range, "8-bit relative value out of range");
	}

	if (cur_section->pc >= (int)(span->org + span->allocated)) {
		span->allocated += 128;
		span->data = g_realloc(span->data, span->allocated);
	}

	if (pad) {
		for (int i = 0; i < nbytes; i++) {
			span->data[span->size++] = 0;
		}
	} else for (int i = 1; i <= nbytes; i++) {
		span->data[span->size++] = (output >> ((nbytes-i)*8)) & 0xff;
	}

	if (cur_section->pc > 0xffff) {
		error(error_type_out_of_range, "assembling beyond addressable memory");
	}

	va_end(ap);
}
