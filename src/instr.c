/*

asm6809, a Motorola 6809 cross assembler
Copyright 2013-2014 Ciaran Anscomb

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

*/

#include "config.h"

#include <assert.h>
#include <stdlib.h>

#include "array.h"
#include "assemble.h"
#include "error.h"
#include "eval.h"
#include "instr.h"
#include "node.h"
#include "opcode.h"
#include "register.h"
#include "section.h"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
 * Inherent instructions.  No arguments.
 */

void instr_inherent(struct opcode const *op, struct node const *args) {
	int nargs = node_array_count(args);
	if (nargs != 0) {
		error(error_type_syntax, "unexpected argument");
	}
	SECTION_EMIT_OP_IMMEDIATE(op);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
 * Inherent.
 */

void instr_immediate(struct opcode const *op, struct node const *args) {
	int nargs = node_array_count(args);
	struct node **arga = node_array_of(args);
	if (nargs != 1) {
		error(error_type_syntax, "invalid number of arguments");
		return;
	}
	SECTION_EMIT_OP_IMMEDIATE(op);
	if (node_type_of(arga[0]) == node_type_int) {
		if ((op->type & OPCODE_EXT_TYPE) == OPCODE_IMM8)
			SECTION_EMIT_IMM8(arga[0]->data.as_int);
		else if ((op->type & OPCODE_EXT_TYPE) == OPCODE_IMM16)
			SECTION_EMIT_IMM16(arga[0]->data.as_int);
		else
			SECTION_EMIT_IMM32(arga[0]->data.as_int);
	} else {
		if ((op->type & OPCODE_EXT_TYPE) == OPCODE_IMM8)
			SECTION_EMIT_PAD(1);
		else if ((op->type & OPCODE_EXT_TYPE) == OPCODE_IMM16)
			SECTION_EMIT_PAD(2);
		else
			SECTION_EMIT_PAD(4);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
 * Relative addressing.
 */

void instr_rel(struct opcode const *op, struct node const *args) {
	int nargs = node_array_count(args);
	struct node **arga = node_array_of(args);
	if (nargs != 1) {
		error(error_type_syntax, "invalid number of arguments");
		return;
	}
	SECTION_EMIT_OP_IMMEDIATE(op);
	_Bool nowarn_rel16 = (node_attr_of(arga[0]) == node_attr_16bit);
	if (node_type_of(arga[0]) == node_type_int) {
		if ((op->type & OPCODE_EXT_TYPE) == OPCODE_REL8)
			SECTION_EMIT_REL8(arga[0]->data.as_int);
		else
			SECTION_EMIT_REL16(arga[0]->data.as_int, nowarn_rel16);
	} else {
		if ((op->type & OPCODE_EXT_TYPE) == OPCODE_REL8)
			SECTION_EMIT_PAD(1);
		else
			SECTION_EMIT_PAD(2);
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
 * Indexed addressing.
 */

/* Check the offset argument in indexed addressing */

#define FLAG_XYUS  (0 << 0)  // OR in the register ID
#define FLAG_PC    (1 << 0)  // constant offset from PC
#define FLAG_PCR   (2 << 0)  // relative offset from PC
#define FLAG_W     (3 << 0)  // affects indirect postbytes
#define FLAG_TYPE  (3 << 0)

#define FLAG_5BIT  (1 << 2)  // value fits in 5 bits signed
#define FLAG_8BIT  (1 << 3)  // value fits in 8 bits signed
#define FLAG_16BIT (1 << 4)
#define FLAG_SIZE  (7 << 2)

#define FLAG_NI    (1 << 5)  // non-indirect allowed
#define FLAG_I     (1 << 6)  // indirect allowed
#define FLAG_WI    (1 << 7)  // warn if indirect used (error for 6309)
#define FLAG_NI_I  (FLAG_NI|FLAG_I)
#define FLAG_NI_WI (FLAG_NI_I|FLAG_WI)

/* Map index register to FLAG_TYPE and (for X, Y, U & S) the value to OR into
 * postbyte. */

static struct {
	enum reg_id reg_id;
	int pbyte_xyus;
	int flags;
} idx_regs[] = {
	{ REG_X, 0x00, FLAG_XYUS },
	{ REG_Y, 0x20, FLAG_XYUS },
	{ REG_U, 0x40, FLAG_XYUS },
	{ REG_S, 0x60, FLAG_XYUS },
	{ REG_PC, 0, FLAG_PC },
	{ REG_PCR, 0, FLAG_PCR },
	{ REG_W, 0, FLAG_W },
	{ .reg_id = REG_INVALID }
};

/* Big table of indexed addressing modes defining all the 2-arg modes.  All
 * of off_type, off_reg, idx_attr and the indirect flag in flags must
 * match.  If postbyte then == -1, each of 5-bit, 8-bit and 16-bit versions are
 * tried in turn (if the appropriate bit is set in flags). */

static struct {
	enum node_type off_type;
	enum reg_id off_reg;
	enum node_attr idx_attr;
	int postbyte;
	unsigned flags;
} indexed_modes[] = {
	/* Constant offset from R */
	{ node_type_empty, REG_INVALID, node_attr_none, 0x84, FLAG_XYUS|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none, 0x00, FLAG_XYUS|FLAG_5BIT|FLAG_NI },
	{ node_type_int,   REG_INVALID, node_attr_none, 0x88, FLAG_XYUS|FLAG_8BIT|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none, 0x89, FLAG_XYUS|FLAG_16BIT|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none,   -1, FLAG_XYUS|FLAG_5BIT|FLAG_8BIT|FLAG_16BIT|FLAG_NI },
	{ node_type_int,   REG_INVALID, node_attr_none,   -1, FLAG_XYUS|FLAG_8BIT|FLAG_16BIT|FLAG_I },

	/* Accumulator offset from R */
	{ node_type_reg, REG_A, node_attr_none, 0x86, FLAG_XYUS|FLAG_NI_I },
	{ node_type_reg, REG_B, node_attr_none, 0x85, FLAG_XYUS|FLAG_NI_I },
	{ node_type_reg, REG_D, node_attr_none, 0x8b, FLAG_XYUS|FLAG_NI_I },

	/* Auto increment/decrement R */
	{ node_type_empty, REG_INVALID, node_attr_postinc,  0x80, FLAG_XYUS|FLAG_NI_WI },
	{ node_type_empty, REG_INVALID, node_attr_postinc2, 0x81, FLAG_XYUS|FLAG_NI_I },
	{ node_type_empty, REG_INVALID, node_attr_predec,   0x82, FLAG_XYUS|FLAG_NI_WI },
	{ node_type_empty, REG_INVALID, node_attr_predec2,  0x83, FLAG_XYUS|FLAG_NI_I },

	/* Constant offset from PC */
	{ node_type_empty, REG_INVALID, node_attr_none, 0x8c, FLAG_PC|FLAG_8BIT|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none, 0x8c, FLAG_PC|FLAG_8BIT|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none, 0x8d, FLAG_PC|FLAG_16BIT|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none,   -1, FLAG_PC|FLAG_8BIT|FLAG_16BIT|FLAG_NI },
	{ node_type_int,   REG_INVALID, node_attr_none,   -1, FLAG_PC|FLAG_16BIT|FLAG_I },
	{ node_type_int,   REG_INVALID, node_attr_none, 0x8c, FLAG_PCR|FLAG_8BIT|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none, 0x8d, FLAG_PCR|FLAG_16BIT|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none,   -1, FLAG_PCR|FLAG_8BIT|FLAG_16BIT|FLAG_NI },
	{ node_type_int,   REG_INVALID, node_attr_none,   -1, FLAG_PCR|FLAG_16BIT|FLAG_I },

	/* Offset from W (6309 extensions) */
	{ node_type_empty, REG_INVALID, node_attr_none,     0x8f, FLAG_W|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none,     0xaf, FLAG_W|FLAG_16BIT|FLAG_NI_I },
	{ node_type_int,   REG_INVALID, node_attr_none,     0xaf, FLAG_W|FLAG_16BIT|FLAG_NI_I },
	{ node_type_empty, REG_INVALID, node_attr_postinc2, 0xcf, FLAG_W|FLAG_NI_I },
	{ node_type_empty, REG_INVALID, node_attr_predec2,  0xef, FLAG_W|FLAG_NI_I },

	/* Accumulator offset from R (6309 extensions) */
	{ node_type_reg, REG_E, node_attr_none, 0x87, FLAG_XYUS|FLAG_NI_I },
	{ node_type_reg, REG_F, node_attr_none, 0x8a, FLAG_XYUS|FLAG_NI_I },
	{ node_type_reg, REG_W, node_attr_none, 0x8e, FLAG_XYUS|FLAG_NI_I },
};

static void instr_indexed2(_Bool indirect, struct node *arg0, struct node *arg1) {

	enum node_type off_type = node_type_of(arg0);
	enum node_attr off_attr = node_attr_of(arg0);
	enum node_type idx_type = node_type_of(arg1);
	enum node_attr idx_attr = node_attr_of(arg1);

	int off_value = 0;
	enum reg_id off_reg = REG_INVALID;

	switch (off_type) {
	case node_type_undef:
		SECTION_EMIT_PAD(3);
		return;
	case node_type_empty:
		break;
	case node_type_int:
		if (off_attr != node_attr_none && off_attr != node_attr_5bit &&
		    off_attr != node_attr_8bit && off_attr != node_attr_16bit) {
			goto invalid_mode;
		}
		off_value = arg0->data.as_int;
		if (off_value == 0 && off_attr == node_attr_none)
			off_type = node_type_empty;
		break;
	case node_type_reg:
		if (off_attr != node_attr_none)
			goto invalid_mode;
		off_reg = arg0->data.as_reg;
		break;
	default:
		goto invalid_mode;
	}

	enum reg_id idx_reg = REG_INVALID;

	switch (idx_type) {
	case node_type_undef:
		SECTION_EMIT_PAD(3);
		return;
	case node_type_reg:
		if (idx_attr != node_attr_none && idx_attr != node_attr_postinc &&
		    idx_attr != node_attr_postinc2 && idx_attr != node_attr_predec &&
		    idx_attr != node_attr_predec2) {
			goto invalid_mode;
		}
		idx_reg = arg1->data.as_reg;
		break;
	default:
		goto invalid_mode;
	}

	int pbyte_xyus = 0;
	int idx_flags = -1;

	for (unsigned i = 0; i < ARRAY_N_ELEMENTS(idx_regs); i++) {
		if (idx_regs[i].reg_id == REG_INVALID) {
			error(error_type_syntax, "invalid index register");
			return;
		}
		if (idx_regs[i].reg_id == idx_reg) {
			pbyte_xyus = idx_regs[i].pbyte_xyus;
			idx_flags = idx_regs[i].flags;
			break;
		}
	}

	for (unsigned i = 0; i < ARRAY_N_ELEMENTS(indexed_modes); i++) {
		int flags_type = indexed_modes[i].flags & FLAG_TYPE;
		int flags_size = indexed_modes[i].flags & FLAG_SIZE;
		_Bool flags_rel = (flags_type == FLAG_PCR);
		_Bool flags_5bit = flags_size & FLAG_5BIT;
		_Bool flags_8bit = flags_size & FLAG_8BIT;
		_Bool flags_16bit = flags_size & FLAG_16BIT;
		int pbyte = indexed_modes[i].postbyte;

		if (off_type != indexed_modes[i].off_type)
			continue;
		if (flags_size == FLAG_5BIT && off_attr != node_attr_5bit)
			continue;
		if (flags_size == FLAG_8BIT && off_attr != node_attr_8bit)
			continue;
		if (flags_size == FLAG_16BIT && off_attr != node_attr_16bit)
			continue;
		if (off_type == node_type_reg && off_reg != indexed_modes[i].off_reg)
			continue;

		if (idx_flags != flags_type)
			continue;

		if (idx_attr != indexed_modes[i].idx_attr)
			continue;

		if (!indirect && !(indexed_modes[i].flags & FLAG_NI))
			continue;
		if (indirect && !(indexed_modes[i].flags & FLAG_I))
			continue;
		if (indirect && (indexed_modes[i].flags & FLAG_WI))
			error(error_type_illegal, "illegal indexed addressing form");

		/* If the postbyte in the table is -1, that indicates a need to try
		 * the value against each of 5-bit, 8-bit and 16-bit offsets. */
		if (pbyte == -1) {
			assert(off_type == node_type_int);
			struct node *arg0c = node_new_int(off_value);
			int try;
			int pc = cur_section->pc;
			if (flags_5bit) {
				try = flags_rel ? (off_value - (pc + 1)) : off_value;
				if (try >= -16 && try <= 15) {
					arg0c = node_set_attr(arg0c, node_attr_5bit);
					instr_indexed2(indirect, arg0c, arg1);
					node_free(arg0c);
					return;
				}
			}
			if (flags_8bit) {
				try = flags_rel ? (off_value - (pc + 2)) : off_value;
				if (try >= -128 && try <= 127) {
					arg0c = node_set_attr(arg0c, node_attr_8bit);
					instr_indexed2(indirect, arg0c, arg1);
					node_free(arg0c);
					return;
				}
			}
			if (flags_16bit) {
				arg0c = node_set_attr(arg0c, node_attr_16bit);
				instr_indexed2(indirect, arg0c, arg1);
				node_free(arg0c);
				return;
			}
			node_free(arg0c);
			goto invalid_mode;
		}

		pbyte |= pbyte_xyus;
		if (flags_type == FLAG_W)
			pbyte += (indirect ? 1 : 0);
		else
			pbyte |= (indirect ? 0x10 : 0);
		if (flags_5bit)
			pbyte |= (off_value & 0x1f);
		SECTION_EMIT_IMM8(pbyte);
		if (flags_5bit)
			return;

		if (!flags_rel) {
			if (flags_8bit)
				SECTION_EMIT_IMM8(off_value);
			if (flags_16bit)
				SECTION_EMIT_IMM16(off_value);
		} else {
			if (flags_8bit)
				SECTION_EMIT_REL8(off_value);
			if (flags_16bit)
				SECTION_EMIT_REL16(off_value, 1);
		}

		return;
	}

invalid_mode:
	error(error_type_syntax, "invalid addressing mode");
	return;
}

void instr_indexed(struct opcode const *op, struct node const *args, int imm8_val) {
	int nargs = node_array_count(args);
	struct node **arga = node_array_of(args);
	int indirect = 0;
	if (nargs == 1 && arga[0]->type == node_type_array) {
		indirect = 0x10;
		nargs = node_array_count(arga[0]);
		arga = arga[0]->data.as_array.args;
	}
	if (nargs < 1 || nargs > 2) {
		error(error_type_syntax, "invalid number of arguments");
		return;
	}
	SECTION_EMIT_OP_INDEXED(op);
	if (imm8_val >= 0)
		SECTION_EMIT_IMM8(imm8_val);

	if (nargs == 1) {
		int pbyte = indirect ? 0x9f : 0x8f;
		int addr = 0;
		switch (node_type_of(arga[0])) {
		case node_type_undef:
			SECTION_EMIT_PAD(3);
			break;
		case node_type_int:
			addr = arga[0]->data.as_int;
			break;
		default:
			error(error_type_syntax, "invalid addressing mode");
			return;
		}
		switch (node_attr_of(arga[0])) {
		case node_attr_none:
		case node_attr_16bit:
			break;
		default:
			error(error_type_syntax, "invalid addressing mode");
			return;
		}
		if (!indirect)
			error(error_type_illegal, "illegal indexed addressing form");
		SECTION_EMIT_IMM8(pbyte);
		SECTION_EMIT_IMM16(addr);
		return;
	}

	instr_indexed2(indirect, arga[0], arga[1]);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
 * Direct and extended addressing.
 */

void instr_address(struct opcode const *op, struct node const *args, int imm8_val) {
	int nargs = node_array_count(args);
	struct node **arga = node_array_of(args);
	if (nargs < 1 || nargs > 2 || (!(op->type & OPCODE_INDEXED) && nargs > 1)) {
		error(error_type_syntax, "invalid number of arguments");
		return;
	}
	if (nargs == 2 || (nargs == 1 && node_type_of(arga[0]) == node_type_array)) {
		instr_indexed(op, args, imm8_val);
		return;
	}

	struct node *arg = eval_int(arga[0]);
	enum node_attr attr = node_attr_16bit;
	unsigned addr = cur_section->pc;
	if (arg) {
		attr = arg->attr;
		addr = arg->data.as_int & 0xffff;
		node_free(arg);
	}

	if ((op->type & OPCODE_DIRECT)) {
		if (attr == node_attr_8bit ||
		    (attr == node_attr_none && (cur_section->dp == (addr >> 8)))) {
			SECTION_EMIT_OP_DIRECT(op);
			if (imm8_val >= 0)
				SECTION_EMIT_IMM8(imm8_val);
			SECTION_EMIT_IMM8(addr);
			return;
		}
	}

	if ((op->type & OPCODE_EXTENDED)) {
		if (attr == node_attr_16bit || attr == node_attr_none) {
			SECTION_EMIT_OP_EXTENDED(op);
			if (imm8_val >= 0)
				SECTION_EMIT_IMM8(imm8_val);
			SECTION_EMIT_IMM16(addr);
			return;
		}
	}

	instr_indexed(op, args, imm8_val);
}

/*
 * 6309-specific 8-bit immediate in-memory instructions.
 */

void instr_imm8_mem(struct opcode const *op, struct node const *args) {
	int nargs = node_array_count(args);
	struct node **arga = node_array_of(args);
	if (nargs < 2 || nargs > 3 || (!(op->type & OPCODE_INDEXED) && nargs > 2)) {
		error(error_type_syntax, "invalid number of arguments");
		return;
	}
	int imm8_val = 0;
	if (node_type_of(arga[0]) == node_type_int)
		imm8_val = arga[0]->data.as_int;
	struct node *newa = node_new_array();
	newa->data.as_array.nargs = nargs - 1;
	newa->data.as_array.args = arga + 1;
	instr_address(op, newa, imm8_val);
	newa->data.as_array.nargs = 0;
	newa->data.as_array.args = NULL;
	node_free(newa);
}

/*
 * Stack operations.  Operands are a list of registers.  e.g., PSHS, PULU.
 */

void instr_stack(struct opcode const *op, struct node const *args, enum reg_id stack) {
	int nargs = node_array_count(args);
	struct node **arga = node_array_of(args);

	int pbyte = 0;
	for (int i = 0; i < nargs; i++) {
		if (node_type_of(arga[i]) != node_type_reg) {
			error(error_type_syntax, "invalid argument");
			return;
		}
		if (arga[i]->data.as_reg == stack)
			goto invalid_register;
		uint8_t bit = reg_stack_bit[arga[i]->data.as_reg];
		if (bit == 0)
			goto invalid_register;
		pbyte |= bit;
	}

	SECTION_EMIT_OP_IMMEDIATE(op);
	SECTION_EMIT_IMM8(pbyte);
	return;

invalid_register:
	error(error_type_syntax, "invalid register in stack operation");
	return;
}

/*
 * Instructions operating on a pair of registers.  e.g., TFR & EXG.
 */

void instr_pair(struct opcode const *op, struct node const *args) {
	int nargs = node_array_count(args);
	struct node * const *arga = node_array_of(args);
	if (nargs != 2) {
		error(error_type_syntax, "invalid number of arguments");
		return;
	}

	int nibble[2];
	int type[2];
	int size[2];
	for (int i = 0; i < 2; i++) {
		type[i] = node_type_of(arga[i]);
		if (type[i] == node_type_reg) {
			nibble[i] = reg_tfr_nibble[arga[i]->data.as_reg];
			size[i] = reg_tfr_size[arga[i]->data.as_reg];
		} else if (type[i] == node_type_int) {
			int v = arga[i]->data.as_int;
			error(error_type_illegal, "numerical values used in inter-register op");
			if (v < 0 || v > 15) {
				error(error_type_out_of_range, "numeric value for register out of range");
			}
			nibble[i] = v & 15;
			// don't warn about size as well if numeric...
			size[i] = 16 | 8;
		} else if (type[i] != node_type_undef) {
			error(error_type_syntax, "invalid register in inter-register op");
			return;
		}
	}
	if ((size[0] & size[1]) == 0) {
		error(error_type_illegal, "register size mismatch in inter-register op");
	}

	int pbyte = (nibble[0] << 4) | nibble[1];
	SECTION_EMIT_OP_IMMEDIATE(op);
	SECTION_EMIT_IMM8(pbyte);
	return;

invalid_register:
	error(error_type_syntax, "invalid register in inter-register op");
	return;
}

/*
 * 6309 extension TFM.
 */

void instr_tfm(struct opcode const *op, struct node const *args) {
	int nargs = node_array_count(args);
	struct node * const *arga = node_array_of(args);
	if (nargs != 2) {
		error(error_type_syntax, "invalid number of arguments");
		return;
	}

	int nibble[2];
	int type[2];
	int attr[2];
	for (int i = 0; i < 2; i++) {
		type[i] = node_type_of(arga[i]);
		attr[i] = node_attr_of(arga[i]);
		if (type[i] == node_type_reg) {
			int v = reg_tfr_nibble[arga[i]->data.as_reg];
			if (v < 0 || v > 4)
				goto invalid_register;
			nibble[i] = v;
		} else if (type[i] == node_type_int) {
			int v = arga[i]->data.as_int;
			error(error_type_illegal, "numerical values used in TFM op");
			if (v < 0 || v > 4) {
				error(error_type_out_of_range, "numeric value for register out of range");
			}
			nibble[i] = v & 15;
		} else if (type[i] != node_type_undef) {
			error(error_type_syntax, "invalid register in inter-register op");
			return;
		}
	}

	int pbyte = (nibble[0] << 4) | nibble[1];
	int mod = 0;
	if (attr[0] == node_attr_postinc && attr[1] == node_attr_postinc) {
		mod = 0;
	} else if (attr[0] == node_attr_postdec && attr[1] == node_attr_postdec) {
		mod = 1;
	} else if (attr[0] == node_attr_postinc && attr[1] == node_attr_none) {
		mod = 2;
	} else if (attr[0] == node_attr_none && attr[1] == node_attr_postinc) {
		mod = 3;
	} else {
		error(error_type_syntax, "invalid TFM mode");
		return;
	}

	SECTION_EMIT_OP_TFM(op, mod);
	SECTION_EMIT_IMM8(pbyte);
	return;

invalid_register:
	error(error_type_syntax, "invalid register in TFM op");
	return;
}

/*
 * Logical operations between specified bits from a register and a direct
 * address.  6309 extended instructions.
 */

void instr_reg_mem(struct opcode const *op, struct node const *args) {
	int nargs = node_array_count(args);
	struct node **arga = node_array_of(args);
	if (nargs != 4) {
		error(error_type_syntax, "invalid number of arguments");
		return;
	}

	int pbyte = 0xc0;
	switch (node_type_of(arga[0])) {
	case node_type_undef:
		break;
	case node_type_int:
		pbyte |= (arga[0]->data.as_int & 3) << 6;
		error(error_type_illegal, "numerical value used in place of register");
		break;
	case node_type_reg:
		switch (arga[0]->data.as_reg) {
		case REG_CC:
			pbyte = 0;
			break;
		case REG_A:
			pbyte = 0x40;
			break;
		case REG_B:
			pbyte = 0x80;
			break;
		default:
			goto invalid_register;
		}
		break;
	default:
		goto invalid_register;
	}

	if (node_type_of(arga[1]) == node_type_int) {
		if (arga[1]->data.as_int < 0 || arga[1]->data.as_int > 7) {
			error(error_type_out_of_range, "bad source bit");
		} else {
			pbyte |= (arga[1]->data.as_int << 3);
		}
	}
	if (node_type_of(arga[2]) == node_type_int) {
		if (arga[2]->data.as_int < 0 || arga[2]->data.as_int > 7) {
			error(error_type_out_of_range, "bad destination bit");
		} else {
			pbyte |= arga[2]->data.as_int;
		}
	}

	SECTION_EMIT_OP_DIRECT(op);
	SECTION_EMIT_IMM8(pbyte);
	if (node_type_of(arga[3]) == node_type_int) {
		SECTION_EMIT_IMM8(arga[3]->data.as_int);
	} else {
		SECTION_EMIT_PAD(1);
	}
	return;

invalid_register:
	error(error_type_syntax, "invalid register in register-memory operation");
	return;
}
