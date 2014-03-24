/*
 * asm6809 - a 6809 assembler
 * Copyright 2013-2014 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_PROGRAM_H_
#define ASM6809_PROGRAM_H_

/*
 * Track the parsed lines that form files or macros.
 *
 * Memory allocation tips:
 * - Data for lines are allocated when reading in a file.
 * - Macros only reference these lines.
 * - prog_free()ing a file will leave any remaining such references dangling,
 *   so free macros first.
 *
 * Programs are read from file with prog_new_file().  Iterate over these by
 * creating a context with prog_ctx_new() and using prog_ctx_next_line().  Free
 * that context when done.  The context stack is used in error reporting.
 *
 * Tracking exported labels (symbols & macros) is done here too.
 */

/* TODO: properly ref count lines */

#include <stdio.h>

#include <glib.h>

struct node;

enum prog_type {
	prog_type_file,
	prog_type_macro
};

struct prog_line {
	unsigned ref;
	struct node *label;
	struct node *opcode;
	struct node *args;  /* must be of type node_arglist */
	char *text;
};

struct prog {
	enum prog_type type;
	char *name;
	unsigned pass;  // only used to detect macro redefinitions
	GSList *lines;
	GSList **next_new_line;
};

struct prog_ctx {
	struct prog *prog;
	GSList *line;
	unsigned line_number;
};

/* The first element on this stack is the current context. */
extern GSList *prog_ctx_stack;

struct prog *prog_new(enum prog_type type, const char *name);
struct prog *prog_new_file(const char *filename);
struct prog *prog_new_macro(const char *name);
void prog_free(struct prog *f);
void prog_free_all(void);  // for tidying up
struct prog *prog_macro_by_name(const char *name);

struct prog_line *prog_line_new(struct node *label, struct node *opcode, struct node *args);
void prog_line_free(struct prog_line *line);
struct prog_line *prog_line_ref(struct prog_line *line);
void prog_line_set_text(struct prog_line *line, char *text);

struct prog_ctx *prog_ctx_new(struct prog *prog);
void prog_ctx_free(struct prog_ctx *ctx);
void prog_ctx_add_line(struct prog_ctx *ctx, struct prog_line *line);
struct prog_line *prog_ctx_next_line(struct prog_ctx *ctx);
_Bool prog_ctx_end(struct prog_ctx *ctx);

void prog_export(const char *name);
void prog_free_exports(void);
void prog_print_exports(FILE *f);

#endif  /* ASM6809_PROGRAM_H_ */
