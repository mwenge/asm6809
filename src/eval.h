/*
 * asm6809 - a 6809 assembler
 * Copyright 2013-2014 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_EVAL_H_
#define ASM6809_EVAL_H_

struct node;

/* Evaluate a node. */
struct node *eval_node(struct node *n);

/*
 * Cast a node to another of a specific type.
 */

struct node *eval_string(struct node *n);
struct node *eval_float(struct node *n);
struct node *eval_int(struct node *n);

/*
 * These variants free the old node before returning the new.
 */

struct node *eval_float_free(struct node *n);
struct node *eval_int_free(struct node *n);

#endif  /* ASM6809_EVAL_H_ */
