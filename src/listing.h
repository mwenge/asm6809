/*
 * asm6809 - a 6809 assembler
 * Copyright 2013 Ciaran Anscomb
 *
 * See COPYING.GPL for redistribution conditions.
 */

#ifndef ASM6809_LISTING_H_
#define ASM6809_LISTING_H_

/*
 * Produce source listings annotated with assembled code output bytes and
 * address information.
 *
 * Before each pass, listing_free_all() ensures any previous attempts at a
 * listing are cleared.  listing_add_line() does what it says on the tin.
 * listing_print() dumps the listing as it currently stands to file.
 */

struct section_span;

void listing_add_line(int pc, int nbytes, struct section_span *span, char *text);
void listing_print(FILE *f);
void listing_free_all(void);

#endif  /* ASM6809_LISTING_H_ */
