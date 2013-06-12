/*
 * asm6809 - a 6809 assembler
 * Copyright 2013 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_INTERP_H_
#define ASM6809_INTERP_H_

struct node;

/* Push an array of nodes on the positional variable stack. */
void interp_push(struct node *);

/* Remove the current array from the stack. */
void interp_pop(void);

/* Fetch positional variable from the current array. */
struct node *interp_get(int index);

#endif  /* ASM6809_INTERP_H_ */
