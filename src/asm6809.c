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

/* for getopt_long */
#define _GNU_SOURCE

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "asm6809.h"
#include "assemble.h"
#include "error.h"
#include "listing.h"
#include "output.h"
#include "program.h"
#include "section.h"
#include "symbol.h"

struct asm6809_options asm6809_options = {
	.isa = asm6809_isa_6809,
	.max_program_depth = 8,
};

#define OUTPUT_BINARY (0)
#define OUTPUT_DRAGONDOS (1)
#define OUTPUT_COCO (2)
#define OUTPUT_MOTOROLA_SREC (3)
#define OUTPUT_INTEL_HEX (4)

static int output_format = OUTPUT_BINARY;
static char *exec_option = NULL;
static char *output_filename = NULL;
static char *symbol_filename = NULL;
static char *listing_filename = NULL;

static struct option long_options[] = {
	{ "bin", no_argument, &output_format, OUTPUT_BINARY },
	{ "dragondos", no_argument, &output_format, OUTPUT_DRAGONDOS },
	{ "coco", no_argument, &output_format, OUTPUT_COCO },
	{ "srec", no_argument, &output_format, OUTPUT_MOTOROLA_SREC },
	{ "hex", no_argument, &output_format, OUTPUT_INTEL_HEX },
	{ "exec", required_argument, NULL, 'e' },
	{ "output", required_argument, NULL, 'o' },
	{ "listing", required_argument, NULL, 'l' },
	{ "symbols", required_argument, NULL, 's' },
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'V' },
	{ NULL, 0, NULL, 0 }
};

static GSList *files = NULL;

static void helptext(void);
static void versiontext(void);
static void tidy_up(void);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

int main(int argc, char **argv) {

	int c;
	while ((c = getopt_long(argc, argv, "BDCSHe:o:l:s:",
				long_options, NULL)) != -1) {
		switch (c) {
		case 0:
			break;
		case 'B':
			output_format = OUTPUT_BINARY;
			break;
		case 'D':
			output_format = OUTPUT_DRAGONDOS;
			break;
		case 'C':
			output_format = OUTPUT_COCO;
			break;
		case 'S':
			output_format = OUTPUT_MOTOROLA_SREC;
			break;
		case 'H':
			output_format = OUTPUT_INTEL_HEX;
			break;
		case 'e':
			exec_option = optarg;
			break;
		case 'o':
			output_filename = optarg;
			break;
		case 'l':
			listing_filename = optarg;
			break;
		case 's':
			symbol_filename = optarg;
			break;
		case 'h':
			helptext();
			exit(EXIT_SUCCESS);
		case 'V':
			versiontext();
			exit(EXIT_SUCCESS);
		default:
			exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		error(error_type_fatal, "no input files");
		error_print_list();
		exit(EXIT_FAILURE);
	}

	/* Read in each file */
	for (int i = optind; i < argc; i++) {
		struct prog *f = prog_new_file(argv[i]);
		files = g_slist_append(files, f);
	}

	/* Fatal errors? */
	if (error_level >= error_type_syntax) {
		error_print_list();
		tidy_up();
		exit(EXIT_FAILURE);
	}

	/* Attempt to assemble files until consistent */
	for (unsigned pass = 0; pass < 10; pass++) {
		error_clear_all();
		listing_free_all();
		section_set("", pass);
		for (GSList *l = files; l; l = l->next) {
			struct prog *f = l->data;
			assemble_prog(f, pass);
		}
		section_finish_pass(pass);
		/* Only inconsistencies trigger another pass */
		if (error_level != error_type_inconsistent)
			break;
	}

	/* Fatal errors? */
	if (error_level >= error_type_inconsistent) {
		error_print_list();
		tidy_up();
		exit(EXIT_FAILURE);
	}
	/* Otherwise print any warnings */
	error_print_list();

	/* Generate listing file */
	if (listing_filename) {
		FILE *listf = fopen(listing_filename, "wb");
		if (listf) {
			listing_print(listf);
			fclose(listf);
		} else {
			error(error_type_fatal, "%s: %s", listing_filename, strerror(errno));
		}
	}

	// XXX At the moment listing generation must precede output, as
	// coelescing spans might screw with the span data to which the listing
	// refers.  Not a big deal, but needs fixing.

	/* Generate output file */
	if (output_filename) {
		switch (output_format) {
		case OUTPUT_BINARY:
			output_binary(output_filename, exec_option);
			break;
		case OUTPUT_DRAGONDOS:
			output_dragondos(output_filename, exec_option);
			break;
		case OUTPUT_COCO:
			output_coco(output_filename, exec_option);
			break;
		case OUTPUT_MOTOROLA_SREC:
			output_motorola_srec(output_filename, exec_option);
			break;
		case OUTPUT_INTEL_HEX:
			output_intel_hex(output_filename, exec_option);
			break;
		default:
			error(error_type_fatal, "internal: unexpected output format");
			break;
		}
	}

	/* Generate symbols file */
	if (symbol_filename) {
		FILE *symf = fopen(symbol_filename, "wb");
		if (symf) {
			prog_print_exports(symf);
			fclose(symf);
		} else {
			error(error_type_fatal, "%s: %s", symbol_filename, strerror(errno));
		}
	}

	/* Any errors in all that? */
	if (error_level >= error_type_syntax) {
		error_print_list();
		tidy_up();
		exit(EXIT_FAILURE);
	}

	error_print_list();
	tidy_up();
	return EXIT_SUCCESS;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static void helptext(void) {
	puts(
"Usage: asm6809 [OPTION]... SOURCE-FILE...\n"
"Assembles 6809 source code.\n"
"\n"
"  -B, --bin         output to binary file (default)\n"
"  -D, --dragondos   output to DragonDOS binary file\n"
"  -C, --coco        output to CoCo segmented binary file\n"
"  -S, --srec        output to Motorola SREC file\n"
"  -H, --hex         output to Intel hex record file\n"
"  -e, --exec=ADDR   EXEC address (for output formats that support one)\n"
"\n"
"  -o, --output=FILE    set output filename\n"
"  -l, --listing=FILE   create listing file\n"
"  -s, --symbols=FILE   create symbol table\n"
"\n"
"      --help      show this help\n"
"      --version   show program version\n"
"\n"
"If more than one SOURCE-FILE is specified, they are assembled as though\n"
"they were all in one file."
	    );
}

static void versiontext(void) {
	puts("asm6809 " PACKAGE_VERSION);
}

/* Call the various free_all routines to tidy up memory.  Allows us to see
 * what's been missed with valgrind. */

static void tidy_up(void) {
	if (files) {
		g_slist_free(files);
		files = NULL;
	}
	listing_free_all();
	prog_free_all();
	symbol_free_all();
	section_free_all();
}
