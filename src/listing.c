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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "listing.h"
#include "program.h"
#include "section.h"

struct listing_line {
	int pc;
	int nbytes;
	struct section_span *span;
	char *text;
};

static GSList *listing_lines = NULL;

void listing_add_line(int pc, int nbytes, struct section_span *span, char *text) {
	struct listing_line *l = g_malloc(sizeof(*l));
	l->pc = pc;
	l->nbytes = nbytes;
	l->span = span;
	l->text = text;
	listing_lines = g_slist_append(listing_lines, l);
}

void listing_print(FILE *f) {
	for (GSList *ll = listing_lines; ll; ll = ll->next) {
		struct listing_line *l = ll->data;
		int col = 0;
		if (l->pc >= 0) {
			fprintf(f, "%04X  ", l->pc & 0xffff);
			col += 6;
		}
		if (l->nbytes > 0 && l->span && l->span->data) {
			int offset = l->pc - l->span->org;
			for (int i = 0; i < l->nbytes; i++) {
				fprintf(f, "%02X", l->span->data[i+offset] & 0xff);
				col += 2;
			}
		}
		do {
			fputc(' ', f);
			col++;
		} while (col < 22);
		col = 0;
		for (int i = 0; l->text[i]; i++) {
			if (l->text[i] == '\t') {
				do {
					fputc(' ', f);
					col++;
				} while ((col % 8) != 0);
			} else {
				fputc(l->text[i], f);
				col++;
			}
		}
		fputc('\n', f);
	}
}

void listing_free_all(void) {
	while (listing_lines) {
		struct listing_line *l = listing_lines->data;
		listing_lines = g_slist_remove(listing_lines, l);
		g_free(l);
	}
}
