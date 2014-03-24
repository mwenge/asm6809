/*
 * asm6809 - a 6809 assembler
 * Copyright 2013-2014 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_ASSEMBLE_H_
#define ASM6809_ASSEMBLE_H_

struct prog;

/*
 * Assemble a file or macro.
 * */

void assemble_prog(struct prog *file, unsigned pass);

/*
 * Tidy up.
 */

void assemble_free_all(void);

#endif  /* ASM6809_ASSEMBLE_H_ */
