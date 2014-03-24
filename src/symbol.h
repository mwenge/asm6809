/*
 * asm6809 - a 6809 assembler
 * Copyright 2013-2014 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_SYMBOL_H_
#define ASM6809_SYMBOL_H_

#include <glib.h>

struct node;

/*
 * Set a symbol in the current symbol table.  The value is evaluated to a
 * simple type before setting.  If the value already existed from a previous
 * pass, an inconsistency is raised if they do not match.
 */

void symbol_set(const char *key, struct node *value, unsigned pass);

/*
 * Fetch a value from the symbol table.
 */

struct node *symbol_get(const char *key);

void symbol_free_all(void);

GHashTable *symbol_local_table_new(void);
struct node *symbol_local_backref(GHashTable *table, long key, unsigned line_number);
struct node *symbol_local_fwdref(GHashTable *table, long key, unsigned line_number);
void symbol_local_set(GHashTable *table, long key, unsigned line_number, struct node *value,
		      unsigned pass);

#endif  /* ASM6809_SYMBOL_H_ */
