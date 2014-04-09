/*

asm6809, a Motorola 6809 cross assembler
Copyright 2013-2014 Ciaran Anscomb

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

*/

#ifndef ASM6809_INSTR_H_
#define ASM6809_INSTR_H_

/*
 * Called from assemble_prog() as appropriate.  Each generates appropriate
 * machine code bytes and emits them to the current section.
 */

#include "register.h"

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
