%{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "error.h"
#include "node.h"
#include "program.h"
#include "register.h"

static void yyerror(char *);
void yylex_destroy(void);
int yylex(void);
char *lex_fetch_line(void);
void lex_free_all(void);

extern FILE *yyin;
static struct prog_ctx *cur_ctx = NULL;

struct prog *grammar_parse_file(const char *filename);
%}

%union {
	int as_token;
	long as_int;
	double as_float;
	char *as_string;
	enum reg_id as_reg;
	struct node *as_node;
	struct prog_line *as_line;
	GSList *as_list;
	}

%token WS
%token <as_string> ID INTERP
%token <as_float> FLOAT
%token <as_int> INTEGER BACKREF FWDREF
%token <as_reg> REGISTER
%token <as_string> TEXT
%token <as_token> SHL SHR
%token DELIM
%token DEC2 INC2

%type <as_node> label opcode
%type <as_node> id string
%type <as_node> idpart strpart arg args
%type <as_node> arglist
%type <as_list> idlist strlist
%type <as_node> expr reg
%type <as_line> line

%left '|'
%left '^'
%left '&'
%left SHL SHR
%left '+' '-'
%left '*' '/' '%'
%nonassoc UMINUS '~'
%left '(' ')'

%%

program	:
	| program line '\n'	{ prog_line_set_text($2, lex_fetch_line()); prog_ctx_add_line(cur_ctx, $2); }
	| program error '\n'	{ yyerrok; }
	;

line	: label opcode args	{ $$ = prog_line_new($1, $2, $3); }
	;

label	:			{ $$ = NULL; }
	| INTEGER		{ $$ = node_new_int($1); }
	| id			{ $$ = $1; }
	;

opcode	:			{ $$ = NULL; }
	| WS id			{ $$ = $2; }
	;

args	:			{ $$ = NULL; }
	| WS arglist		{ $$ = $2; }
	;

id	: idlist		{ $$ = node_new_id($1); }
	;

idlist	: idpart		{ $$ = g_slist_append(NULL, $1); }
	| idlist idpart		{ $$ = g_slist_append($1, $2); }
	;

idpart	: ID			{ $$ = node_new_string($1); }
      	| INTERP		{ $$ = node_new_interp($1); }
	;

arglist	: arg			{ $$ = node_array_push(NULL, $1); }
	| arglist ',' arg	{ $$ = node_array_push($1, $3); }
	;

arg	:			{ $$ = node_new_empty(); }
	| '[' arglist ']'	{ $$ = $2; }
	| '#' expr		{ $$ = node_set_attr($2, node_attr_immediate); }
	| SHL expr		{ $$ = node_set_attr($2, node_attr_5bit); }
	| '<' expr		{ $$ = node_set_attr($2, node_attr_8bit); }
	| '>' expr		{ $$ = node_set_attr($2, node_attr_16bit); }
	| '<'			{ $$ = node_new_backref(0); }
	| '>'			{ $$ = node_new_fwdref(0); }
	| reg '+'		{ $$ = node_set_attr($1, node_attr_postinc); }
	| reg INC2		{ $$ = node_set_attr($1, node_attr_postinc2); }
	| '-' reg		{ $$ = node_set_attr($2, node_attr_predec); }
	| DEC2 reg		{ $$ = node_set_attr($2, node_attr_predec2); }
	| reg '-'		{ $$ = node_set_attr($1, node_attr_postdec); }
	| reg			{ $$ = $1; }
	| expr			{ $$ = $1; }
	| string		{ $$ = $1; }
	;

reg	: REGISTER		{ $$ = node_new_reg($1); }
	;

expr	: '(' expr ')'		{ $$ = $2; }
	| '-' expr %prec UMINUS	{ $$ = node_new_oper_1('-', $2); }
	| '+' expr %prec UMINUS	{ $$ = node_new_oper_1('+', $2); }
	| '~' expr		{ $$ = node_new_oper_1('~', $2); }
	| expr '*' expr		{ $$ = node_new_oper_2('*', $1, $3); }
	| expr '/' expr		{ $$ = node_new_oper_2('/', $1, $3); }
	| expr '%' expr		{ $$ = node_new_oper_2('%', $1, $3); }
	| expr '+' expr		{ $$ = node_new_oper_2('+', $1, $3); }
	| expr '-' expr		{ $$ = node_new_oper_2('-', $1, $3); }
	| expr SHL expr		{ $$ = node_new_oper_2(SHL, $1, $3); }
	| expr SHR expr		{ $$ = node_new_oper_2(SHR, $1, $3); }
	| expr '&' expr		{ $$ = node_new_oper_2('&', $1, $3); }
	| expr '^' expr		{ $$ = node_new_oper_2('^', $1, $3); }
	| expr '|' expr		{ $$ = node_new_oper_2('|', $1, $3); }
	| INTEGER		{ $$ = node_new_int($1); }
	| FLOAT			{ $$ = node_new_float($1); }
	| BACKREF		{ $$ = node_new_backref($1); }
	| FWDREF		{ $$ = node_new_fwdref($1); }
	| '*'			{ $$ = node_new_pc(); }
	| id			{ $$ = $1; }
	;

string	: DELIM strlist DELIM	{ $$ = node_new_text($2); }
	;

strlist	: strpart		{ $$ = g_slist_append(NULL, $1); }
	| strlist strpart	{ $$ = g_slist_append($1, $2); }
	;

strpart	: TEXT			{ $$ = node_new_string($1); }
	| INTERP		{ $$ = node_new_interp($1); }
	;

%%

static void yyerror(char *s) {
	// discard line with error - going to fail anyway
	char *l = lex_fetch_line();
	g_free(l);
	cur_ctx->line_number++;
	error(error_type_syntax, "%s", s);
}

struct prog *grammar_parse_file(const char *filename) {
	yyin = fopen(filename, "r");
	if (!yyin) {
		error(error_type_fatal, "file not found: %s", filename);
		return NULL;
	}
	struct prog *prog = prog_new(prog_type_file, filename);
	cur_ctx = prog_ctx_new(prog);
	yyparse();
	prog_ctx_free(cur_ctx);
	cur_ctx = NULL;
	fclose(yyin);
	lex_free_all();
	return prog;
}
