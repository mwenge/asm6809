/*
 * asm6809 - a 6809 assembler
 * Copyright 2013 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_REGISTERS_H_
#define ASM6809_REGISTERS_H_

enum reg_id {
	REG_INVALID = -1,

	REG_CC = 0,
	REG_A,
	REG_B,
	REG_DP,
	REG_X,
	REG_Y,
	REG_U,
	REG_S,
	REG_PC,
	REG_D,

	REG_PCR,

	REG_E,
	REG_F,
	REG_W,
	REG_Q,
	REG_V,
};

/*
 * These functions will only handle registers valid in the current ISA
 */

enum reg_id reg_name_to_id(const char *name);
const char *reg_id_to_name(enum reg_id id);

#endif  /* ASM6809_REGISTERS_H_ */
