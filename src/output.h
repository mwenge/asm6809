/*
 * asm6809 - a 6809 assembler
 * Copyright 2013 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_OUTPUT_H_
#define ASM6809_OUTPUT_H_

/*
 * Write assembled data to a variety of output formats.
 */

/* Output format: Binary. */
void output_binary(const char *filename, const char *exec);

/* Output format: DragonDOS binary. */
void output_dragondos(const char *filename, const char *exec);

/* Output format: CoCo RSDOS binary. */
void output_coco(const char *filename, const char *exec);

/* Output format: Motorola SREC. */
void output_motorola_srec(const char *filename, const char *exec);

/* Output format: Intel HEX. */
void output_intel_hex(const char *filename, const char *exec);

#endif  /* ASM6809_OUTPUT_H_ */
