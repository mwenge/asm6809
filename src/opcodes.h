/*
 * asm6809 - a 6809 assembler
 * Copyright 2013 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_OPCODES_H_
#define ASM6809_OPCODES_H_

/*
 * Each instruction opcode may have many forms.
 *
 * - type: Bits 0, 1 and 2 indicate the presence of direct, indexed and
 *   extended opcode values.  Bits 3-5 indicate how the the immediate opcode
 *   value is to be interpreted.
 *
 * - immediate, direct, indexed, extended: Opcode value for the corresponding
 *   mode.  If bits 8-15 are non-zero, they indicate an instruction page byte.
 */

struct opcode {
	const char *op;
	unsigned char type;
	unsigned short immediate;
	unsigned short direct;
	unsigned short indexed;
	unsigned short extended;
};

#define OPCODE_DIRECT   (1 << 0)
#define OPCODE_INDEXED  (1 << 1)
#define OPCODE_EXTENDED (1 << 2)
#define OPCODE_IMM      (15 << 3)

#define OPCODE_MEM (OPCODE_DIRECT|OPCODE_INDEXED|OPCODE_EXTENDED)

#define OPCODE_INHERENT (1 << 3)
#define OPCODE_IMM8     (2 << 3)
#define OPCODE_IMM16    (3 << 3)
#define OPCODE_PAIR     (4 << 3)
#define OPCODE_STACKU   (5 << 3)
#define OPCODE_STACKS   (6 << 3)
#define OPCODE_REL8     (7 << 3)
#define OPCODE_REL16    (8 << 3)
#define OPCODE_IMM32    (9 << 3)
#define OPCODE_IMM8_MEM (10 << 3)
#define OPCODE_REG_MEM  (11 << 3)
#define OPCODE_TFM      (12 << 3)

struct opcode *opcode_by_name(const char *name);

#endif  /* ASM6809_OPCODES_H_ */
