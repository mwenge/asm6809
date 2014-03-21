/*
 * Copyright 2013-2014 Ciaran Anscomb
 *
 * This file is part of asm6809.
 *
 * asm6809 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License,
 * or (at your option) any later version.
 *
 * asm6809 is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with asm6809.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "interp.h"
#include "node.h"
#include "slist.h"

static struct slist *interp_stack = NULL;

void interp_push(struct node *n) {
	switch (node_type_of(n)) {
	case node_type_undef:
	case node_type_array:
		break;
	default:
		error(error_type_fatal, "internal: pushing non-array onto interp stack");
		return;
	}
	interp_stack = slist_prepend(interp_stack, node_ref(n));
}

void interp_pop(void) {
	if (!interp_stack) {
		error(error_type_fatal, "internal: popping off empty interp stack");
		return;
	}
	struct node *n = interp_stack->data;
	interp_stack = slist_remove(interp_stack, n);
	node_free(n);
}

struct node *interp_get(int index) {
	if (!interp_stack) {
		error(error_type_syntax, "no positional variables on stack");
		return NULL;
	}
	struct node *args = interp_stack->data;
	int nargs = args ? args->data.as_array.nargs : 0;
	if (index < 1 || index > nargs) {
		error(error_type_syntax, "invalid positional variable: %d", index);
		return NULL;
	}
	struct node *n = args->data.as_array.args[index-1];
	return node_ref(n);
}
