/*

asm6809, a Motorola 6809 cross assembler
Copyright 2013-2014 Ciaran Anscomb

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

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
