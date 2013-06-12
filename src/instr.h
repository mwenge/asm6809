/*
 * asm6809 - a 6809 assembler
 * Copyright 2013 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_INSTR_H_
#define ASM6809_INSTR_H_

/*
 * Called from assemble_prog() as appropriate.  Each generates appropriate
 * machine code bytes and emits them to the current section.
 */

#include "registers.h"

struct node;
struct opcode;

void instr_inherent(struct opcode *op, struct node *args);
void instr_immediate(struct opcode *op, struct node *args);
void instr_rel(struct opcode *op, struct node *args);
void instr_indexed(struct opcode *op, struct node *args);
void instr_address(struct opcode *op, struct node *args);
void instr_stack(struct opcode *op, struct node *args, enum reg_id stack);
void instr_pair(struct opcode *op, struct node *args);

#endif  /* ASM6809_INSTR_H_ */
