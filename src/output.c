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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <glib.h>

#include "error.h"
#include "eval.h"
#include "node.h"
#include "output.h"
#include "section.h"
#include "symbol.h"

/* Helper that dumps a single binary blob to file. */

static void write_single_binary(FILE *f, struct section_span *span) {
	unsigned size = span->size;
	fwrite(span->data, 1, size, f);
}

/* Helper to figure out exec address. */

static unsigned get_exec_addr(const char *exec) {
	if (!exec)
		return 0;

	/* Standard numeric forms */
	if (exec[0] == '$')
		return strtol(exec+1, NULL, 16);
	if (exec[0] == '@')
		return strtol(exec+1, NULL, 8);
	if (exec[0] == '%')
		return strtol(exec+1, NULL, 2);
	if (exec[0] == '0') {
		if (exec[1] == 'x')
			return strtol(exec+2, NULL, 16);
		if (exec[1] == 'b')
			return strtol(exec+2, NULL, 2);
	}
	if (exec[0] >= '0' && exec[0] <= '9')
		return strtol(exec, NULL, 10);

	struct node *n = symbol_get(exec);
	unsigned ret = 0;
	if (n) {
		ret = n->data.as_int & 0xffff;
		node_free(n);
	} else {
		error(error_type_fatal, "exec symbol '%s' not defined", exec);
	}
	return ret;
}

/* Output format: Plain binary.  All coalesced into one big blob. */

void output_binary(const char *filename, const char *exec) {
	if (exec) {
		error(error_type_fatal, "exec address not valid for binary output");
		return;
	}

	FILE *f = fopen(filename, "wb");
	if (!f)
		return;

	struct section *sect = section_coalesce_all(1);
	struct section_span *span = sect->spans->data;
	write_single_binary(f, span);

	section_free(sect);
	fclose(f);
}

/* Output format: DragonDOS binary. */

void output_dragondos(const char *filename, const char *exec) {
	unsigned exec_addr = get_exec_addr(exec);

	FILE *f = fopen(filename, "wb");
	if (!f)
		return;

	struct section *sect = section_coalesce_all(1);
	struct section_span *span = sect->spans->data;
	unsigned put = span->put;
	unsigned size = span->size;
	if (!exec_addr)
		exec_addr = put;

	fputc(0x55, f);
	fputc(0x02, f);
	fputc((put >> 8) & 0xff, f);
	fputc(put & 0xff, f);
	fputc((size >> 8) & 0xff, f);
	fputc(size & 0xff, f);
	fputc((exec_addr >> 8) & 0xff, f);
	fputc(exec_addr  & 0xff, f);
	fputc(0xaa, f);
	write_single_binary(f, span);

	section_free(sect);
	fclose(f);
}

/* Output format: CoCo RSDOS binary. */

/*
 * TODO: Currently all sections are merged.  Would be preferable to have an
 * ordered list of sections, each of which can be coalesced and dumped in
 * order.
 */

void output_coco(const char *filename, const char *exec) {
	unsigned exec_addr = get_exec_addr(exec);

	FILE *f = fopen(filename, "wb");
	if (!f)
		return;

	struct section *sect = section_coalesce_all(0);

	for (GSList *l = sect->spans; l; l = l->next) {
		struct section_span *span = l->data;
		unsigned put = span->put;
		unsigned size = span->size;
		fputc(0x00, f);
		fputc((size >> 8) & 0xff, f);
		fputc(size & 0xff, f);
		fputc((put >> 8) & 0xff, f);
		fputc(put & 0xff, f);
		fwrite(span->data, 1, size, f);
	}

	if (exec) {
		fputc(0xff, f);
		fputc(0x00, f);
		fputc(0x00, f);
		fputc((exec_addr >> 8) & 0xff, f);
		fputc(exec_addr  & 0xff, f);
	}

	section_free(sect);
	fclose(f);
}

/* Output format: Motorola SREC. */

void output_motorola_srec(const char *filename, const char *exec) {
	unsigned exec_addr = get_exec_addr(exec);

	FILE *f = fopen(filename, "wb");
	if (!f)
		return;

	struct section *sect = section_coalesce_all(0);

	for (GSList *l = sect->spans; l; l = l->next) {
		struct section_span *span = l->data;
		unsigned put = span->put;
		unsigned size = span->size;
		unsigned base = 0;
		while (size > 0) {
			unsigned nbytes = (size > 32) ? 32 : size;
			unsigned sum = nbytes + put + (put >> 8);
			fprintf(f, "S1%02X%04X", nbytes + 3, put);
			for (unsigned i = 0; i < nbytes; i++) {
				fprintf(f, "%02X", (unsigned)span->data[base + i]);
				sum += span->data[base + i];
			}
			sum = ~sum & 0xff;
			fprintf(f, "%02X\n", sum);
			put += nbytes;
			base += nbytes;
			size -= nbytes;
		}
	}

	if (exec) {
		unsigned sum = exec_addr + (exec_addr >> 8) + 3;
		fprintf(f, "S903%04X%02X\n", exec_addr, ~sum & 0xff);
	}

	section_free(sect);
	fclose(f);
}

/* Output format: Intel HEX. */

void output_intel_hex(const char *filename, const char *exec) {
	unsigned exec_addr = get_exec_addr(exec);

	FILE *f = fopen(filename, "wb");
	if (!f)
		return;

	struct section *sect = section_coalesce_all(0);

	for (GSList *l = sect->spans; l; l = l->next) {
		struct section_span *span = l->data;
		unsigned put = span->put;
		unsigned size = span->size;
		unsigned base = 0;
		while (size > 0) {
			unsigned nbytes = (size > 32) ? 32 : size;
			unsigned sum = nbytes + put + (put >> 8);
			fprintf(f, ":%02X%04X00", nbytes, put);
			for (unsigned i = 0; i < nbytes; i++) {
				fprintf(f, "%02X", (unsigned)span->data[base + i]);
				sum += span->data[base + i];
			}
			sum = (~sum + 1) & 0xff;
			fprintf(f, "%02X\n", sum);
			put += nbytes;
			base += nbytes;
			size -= nbytes;
		}
	}

	if (!exec) {
		fprintf(f, ":00000001FF\n");
	} else {
		unsigned sum = exec_addr + (exec_addr >> 8) + 1;
		fprintf(f, ":00%04X01%02X\n", exec_addr, (~sum + 1) & 0xff);
	}

	section_free(sect);
	fclose(f);
}
