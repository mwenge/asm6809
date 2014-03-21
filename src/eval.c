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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xalloc.h"
#include "xvasprintf.h"

#include "error.h"
#include "eval.h"
#include "interp.h"
#include "node.h"
#include "register.h"
#include "section.h"
#include "slist.h"
#include "symbol.h"

#include "grammar.h"

static struct node *eval_node_oper_1(struct node *n);
static struct node *eval_node_oper_2(struct node *n);

/* Evaluate a node.  The return value will be a new node of a base type -
 * possibly just a reference to the argument node.  The exception is arrays,
 * for which a new array is returned with each element evaluated according to
 * the same rules. */

struct node *eval_node(struct node *n) {
	struct node *tmp1;

	if (!n)
		return NULL;

	enum node_attr attr = n->attr;

	switch (n->type) {

	/* Base types - return a reference to self */
	case node_type_empty:
	case node_type_int:
	case node_type_float:
	case node_type_reg:
	case node_type_string:
		return node_ref(n);

	/* Program counter */
	case node_type_pc:
		return node_set_attr(node_new_int(cur_section->pc), attr);

	/* Backref/fwdref need to search for local label */
	case node_type_backref:
		return node_set_attr(symbol_local_backref(cur_section->local_labels, n->data.as_int, cur_section->line_number), attr);
	case node_type_fwdref:
		return node_set_attr(symbol_local_fwdref(cur_section->local_labels, n->data.as_int, cur_section->line_number), attr);

	/* Interpolate variable */
	case node_type_interp:
		return interp_get(strtol(n->data.as_string, NULL, 10));

	/* Identifier.  Either a single positional variable to be looked up
	 * directly, or a list of strings, positional variables or register
	 * names to be pasted together to form a symbol name, which is then
	 * fetched and evaluated.  */
	case node_type_id:
		if (n->data.as_list->next == NULL) {
			struct node *arg = n->data.as_list->data;
			if (arg && arg->type == node_type_interp)
				return node_set_attr_if(eval_node(arg), attr);
		}
		if ((tmp1 = eval_string(n))) {
			struct node *tmp2 = symbol_get(tmp1->data.as_string);
			node_free(tmp1);
			tmp1 = eval_node(tmp2);
			node_free(tmp2);
			return node_set_attr_if(tmp1, attr);
		}
		return NULL;

	/* A list of strings and positional variables to be pasted together to
	 * form a piece of text. */
	case node_type_text:
		return node_set_attr(eval_string(n), attr);

	/* Apply operator to arguments */
	case node_type_oper:
		if (n->data.as_oper.nargs == 2)
			return node_set_attr(eval_node_oper_2(n), attr);
		if (n->data.as_oper.nargs == 1)
			return node_set_attr(eval_node_oper_1(n), attr);
		error(error_type_fatal, "internal: bad number of args (%d) for operator", n->data.as_oper.nargs);
		return NULL;

	/* Evaluating an array returns another array */
	case node_type_array:
		{
			struct node **arga = n->data.as_array.args;
			int nargs = n->data.as_array.nargs;
			struct node *new = NULL;
			for (int i = 0; i < nargs; i++) {
				struct node *tmp;
				tmp = eval_node(arga[i]);
				new = node_array_push(new, tmp);
			}
			return node_set_attr(new, attr);
		}

	default:
		break;
	}

	error(error_type_fatal, "internal: eval of unhandled type %d", n->type);
	return NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

/*
 * Cast a node to another of a specific type.
 */

struct node *eval_string(struct node *n) {
	if (!n)
		return NULL;
	if (n->type == node_type_string)
		return node_ref(n);
	if (n->type != node_type_id && n->type != node_type_text)
		return NULL;
	enum node_attr attr = node_attr_of(n);
	struct node *out = node_new_string(NULL);
	int size = 0;
	for (struct slist *l = n->data.as_list; l; l = l->next) {
		struct node *elem = l->data;
		struct node *tmp;
		if (!(tmp = eval_node(elem))) {
			node_free(out);
			return NULL;
		}
		char *addtext;
		switch (tmp->type) {
		case node_type_string:
			addtext = xstrdup(tmp->data.as_string);
			break;
		case node_type_int:
			addtext = xasprintf("%ld", tmp->data.as_int);
			break;
		case node_type_reg:
			if (tmp->attr != node_attr_none) {
				node_free(tmp);
				node_free(out);
				return NULL;
			}
			addtext = xstrdup(reg_id_to_name(tmp->data.as_reg));
			break;
		default:
			node_free(tmp);
			node_free(out);
			return NULL;
		}
		int add = strlen(addtext);
		out->data.as_string = xrealloc(out->data.as_string, size + add + 1);
		out->data.as_string[size] = 0;
		size += add;
		strcat(out->data.as_string, addtext);
		free(addtext);
		node_free(tmp);
	}
	return node_set_attr(out, attr);
}

struct node *eval_float(struct node *n) {
	switch (node_type_of(n)) {
	case node_type_float:
		return node_ref(n);
	case node_type_int:
		return node_set_attr(node_new_float(n->data.as_int), n->attr);
	default:
		break;
	}
	return NULL;
}

struct node *eval_int(struct node *n) {
	switch (node_type_of(n)) {
	case node_type_float:
		return node_set_attr(node_new_int(n->data.as_float), n->attr);
	case node_type_int:
		return node_ref(n);
	default:
		break;
	}
	return NULL;
}

/* Free original node before returning the cast value.  If ref count is 1,
 * these can modify in place, as the node_free() would otherwise discard the
 * space. */

struct node *eval_float_free(struct node *n) {
	if (!n)
		return NULL;
	if (n->ref != 1) {
		struct node *ret = eval_float(n);
		node_free(n);
		return ret;
	}
	if (n->type == node_type_int) {
		n->type = node_type_float;
		n->data.as_float = n->data.as_int;
		return n;
	}
	if (n->type == node_type_float)
		return n;
	node_free(n);
	return NULL;
}

struct node *eval_int_free(struct node *n) {
	if (!n)
		return NULL;
	if (n->ref != 1) {
		struct node *ret = eval_int(n);
		node_free(n);
		return ret;
	}
	if (n->type == node_type_float) {
		n->type = node_type_int;
		n->data.as_int = n->data.as_float;
		return n;
	}
	if (n->type == node_type_int)
		return n;
	node_free(n);
	return NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

static struct node *eval_node_oper_1(struct node *n) {
	struct node *ret = NULL;
	struct node *arg;

	if (!(arg = eval_node(n->data.as_oper.args[0])))
		return NULL;

	switch (n->data.as_oper.oper) {

	case '-':
		switch (node_type_of(arg)) {
		case node_type_int:
			ret = node_new_int(-arg->data.as_int);
			break;
		case node_type_float:
			ret = node_new_float(-arg->data.as_float);
			break;
		default:
			break;
		}
		break;

	case '+':
		switch (node_type_of(arg)) {
		case node_type_int:
		case node_type_float:
			return arg;
		default:
			break;
		}
		break;

	case '~':
		arg = eval_int_free(arg);
		if (arg) {
			ret = node_new_int(~arg->data.as_int);
		}
		break;

	default:
		error(error_type_fatal, "internal: unknown 1-arg operator %d", n->data.as_oper.oper);
		break;
	}

	node_free(arg);
	return ret;
}

static struct node *eval_node_oper_2(struct node *n) {
	struct node *leftn;
	struct node *rightn;
	struct node *ret;

	if (!(leftn = eval_node(n->data.as_oper.args[0])))
		return NULL;
	if (!(rightn = eval_node(n->data.as_oper.args[1]))) {
		node_free(leftn);
		return NULL;
	}

	_Bool int_only = (leftn->type == node_type_int && rightn->type == node_type_int);

	switch (n->data.as_oper.oper) {

	/* Operators that can be integer-only or cast to float */
	case '*': case '/':
	case '+': case '-':

		if (int_only && n->data.as_oper.oper != '/') {
			switch (n->data.as_oper.oper) {
			case '*':
				ret = node_new_int(leftn->data.as_int * rightn->data.as_int);
				break;
			case '+':
				ret = node_new_int(leftn->data.as_int + rightn->data.as_int);
				break;
			case '-':
				ret = node_new_int(leftn->data.as_int - rightn->data.as_int);
				break;
			default:
				ret = NULL;
				break;
			}
			node_free(leftn);
			node_free(rightn);
			return ret;
		}

		if (!(leftn = eval_float_free(leftn))) {
			node_free(rightn);
			return NULL;
		}
		if (!(rightn = eval_float_free(rightn))) {
			node_free(leftn);
			return NULL;
		}

		switch (n->data.as_oper.oper) {
		case '*':
			ret = node_new_float(leftn->data.as_float * rightn->data.as_float);
			break;
		case '/':
			ret = node_new_float(leftn->data.as_float / rightn->data.as_float);
			break;
		case '+':
			ret = node_new_float(leftn->data.as_float + rightn->data.as_float);
			break;
		case '-':
			ret = node_new_float(leftn->data.as_float - rightn->data.as_float);
			break;
		default:
			ret = NULL;
			break;
		}
		node_free(leftn);
		node_free(rightn);
		return ret;

	/* Operators that only apply to integers */
	case '%': case SHL: case SHR:
	case '&': case '^': case '|':

		if (!(leftn = eval_int_free(leftn))) {
			node_free(rightn);
			return NULL;
		}
		if (!(rightn = eval_int_free(rightn))) {
			node_free(leftn);
			return NULL;
		}

		switch (n->data.as_oper.oper) {
		case '%':
			ret = node_new_int(leftn->data.as_int % rightn->data.as_int);
			break;
		case SHL:
			ret = node_new_int(leftn->data.as_int << rightn->data.as_int);
			break;
		case SHR:
			ret = node_new_int(leftn->data.as_int >> rightn->data.as_int);
			break;
		case '&':
			ret = node_new_int(leftn->data.as_int & rightn->data.as_int);
			break;
		case '^':
			ret = node_new_int(leftn->data.as_int ^ rightn->data.as_int);
			break;
		case '|':
			ret = node_new_int(leftn->data.as_int | rightn->data.as_int);
			break;
		default:
			ret = NULL;
			break;
		}
		node_free(leftn);
		node_free(rightn);
		return ret;

	default:
		error(error_type_fatal, "internal: unknown 2-arg operator %d", n->data.as_oper.oper);
		break;
	}

	node_free(leftn);
	node_free(rightn);
	return NULL;
}
