/*

asm6809, a Motorola 6809 cross assembler
Copyright 2013-2014 Ciaran Anscomb

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

*/

#ifndef ASM6809_SYMBOL_H_
#define ASM6809_SYMBOL_H_

struct dict;
struct node;

/*
 * Set a symbol in the current symbol table.  The value is evaluated to a
 * simple type before setting.  If the value already existed from a previous
 * pass, an inconsistency is raised if they do not match.
 */

void symbol_set(const char *key, struct node *value, unsigned pass);

/* As above but return 1 if inconsistent instead of raising error. */

_Bool symbol_force_set(const char *key, struct node *value, unsigned pass);

/*
 * Fetch a value from the symbol table.
 */

struct node *symbol_try_get(const char *key);
struct node *symbol_get(const char *key);

void symbol_free_all(void);

struct dict *symbol_local_table_new(void);
struct node *symbol_local_backref(struct dict *table, long key, unsigned line_number);
struct node *symbol_local_fwdref(struct dict *table, long key, unsigned line_number);
void symbol_local_set(struct dict *table, long key, unsigned line_number, struct node *value,
		      unsigned pass);

#endif  /* ASM6809_SYMBOL_H_ */
