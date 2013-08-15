%{
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <glib.h>

#include "error.h"
#include "register.h"

#include "grammar.h"

static int delim;

static int id_or_reg(void);
static int read_line(char *buf, int max_size);

char *lex_fetch_line(void);
void lex_free_all(void);

#define YY_INPUT(buf,result,max_size) \
	{ result = read_line(buf, max_size); }

%}

%option noyywrap

%s opcode
%s arg
%s argnonum
%s argnostr
%s string

nzdigit	[1-9]
digit	[0-9]
octdigit	[0-7]
bindigit	[01]
hexdigit	[0-9A-Fa-f]
ws	[ \t]

decimal	{digit}+
word	[A-Za-z_][A-Za-z0-9_]*

%%

<INITIAL>{

\%{bindigit}+	{ yylval.as_int = strtol(yytext+1, NULL, 2); return INTEGER; }
0b{bindigit}+	{ yylval.as_int = strtol(yytext+2, NULL, 2); return INTEGER; }
@{octdigit}+	{ yylval.as_int = strtol(yytext+1, NULL, 8); return INTEGER; }
0{octdigit}+	{ yylval.as_int = strtol(yytext, NULL, 8); return INTEGER; }
{decimal}	{ yylval.as_int = strtol(yytext, NULL, 10); return INTEGER; }
${hexdigit}+	{ yylval.as_int = strtol(yytext+1, NULL, 16); return INTEGER; }
0x{hexdigit}+	{ yylval.as_int = strtol(yytext+2, NULL, 16); return INTEGER; }
'.		{ yylval.as_int = *(yytext+1); return INTEGER; }
\!		{ yylval.as_int = 0; return INTEGER; }

{word}		{ return id_or_reg(); }
&{decimal}	{ yylval.as_string = g_strndup(yytext+1, yyleng-1); return INTERP; }
&\{{decimal}\}	{ yylval.as_string = g_strndup(yytext+2, yyleng-3); return INTERP; }

\r		/* skip CR */
{ws}*[;\*].*	/* skip comment */
{ws}+		{ BEGIN(opcode); return WS; }
{ws}*\n		{ return '\n'; }
.		{ return *yytext; }

}

<opcode>{

{word}		{ return id_or_reg(); }
&{decimal}	{ yylval.as_string = g_strndup(yytext+1, yyleng-1); return INTERP; }
&\{{decimal}\}	{ yylval.as_string = g_strndup(yytext+2, yyleng-3); return INTERP; }


\r		/* skip CR */
{ws}*;.*	/* skip comment */
{ws}+		{ BEGIN(arg); return WS; }
{ws}*\n		{ BEGIN(INITIAL); return '\n'; }
.		{ BEGIN(INITIAL); return *yytext; }

}

<arg,argnostr>{

\%{bindigit}+	{ BEGIN(argnonum); yylval.as_int = strtol(yytext+1, NULL, 2); return INTEGER; }
0b{bindigit}+	{ BEGIN(argnonum); yylval.as_int = strtol(yytext+2, NULL, 2); return INTEGER; }
@{octdigit}+	{ BEGIN(argnonum); yylval.as_int = strtol(yytext+1, NULL, 8); return INTEGER; }
0{octdigit}+	{ BEGIN(argnonum); yylval.as_int = strtol(yytext, NULL, 8); return INTEGER; }
{decimal}	{ BEGIN(argnonum); yylval.as_int = strtol(yytext, NULL, 10); return INTEGER; }
${hexdigit}+	{ BEGIN(argnonum); yylval.as_int = strtol(yytext+1, NULL, 16); return INTEGER; }
0x{hexdigit}+	{ BEGIN(argnonum); yylval.as_int = strtol(yytext+2, NULL, 16); return INTEGER; }
'.		{ BEGIN(argnonum); yylval.as_int = *(yytext+1); return INTEGER; }
[0-9]*\.[0-9]+	|
{decimal}\.	{ BEGIN(argnonum); yylval.as_float = strtod(yytext, NULL); return FLOAT; }

}

<arg>{

[/"]		{
			delim = *yytext;
			BEGIN(string);
			return DELIM;
		}

}

<argnonum,argnostr>{
[/"]		{ BEGIN(argnostr); return *yytext; }
}

<arg,argnonum,argnostr>{

{word}		{ BEGIN(argnonum); return id_or_reg(); }
&{decimal}	{ BEGIN(argnonum); yylval.as_string = g_strndup(yytext+1, yyleng-1); return INTERP; }
&\{{decimal}\}	{ BEGIN(argnonum); yylval.as_string = g_strndup(yytext+2, yyleng-3); return INTERP; }

\+\+		{ BEGIN(argnonum); return INC2; }
\-\-		{ BEGIN(argnonum); return DEC2; }

{decimal}[bB]	{ BEGIN(argnonum); yylval.as_int = strtol(yytext, NULL, 10); return BACKREF; }
{decimal}[fF]	{ BEGIN(argnonum); yylval.as_int = strtol(yytext, NULL, 10); return FWDREF; }

"<<"		{ BEGIN(argnostr); return SHL; }
">>"		{ BEGIN(argnostr); return SHR; }

[-+&\|\~\*()#<>\%\[\]]	{ BEGIN(argnostr); return *yytext; }

,		{ BEGIN(arg); return *yytext; }

\r		/* skip CR */
{ws}*;.*	/* skip comment */
{ws}+		/* skip whitespace */
\n		|
.		{ BEGIN(INITIAL); return *yytext; }

}

<string>{

[/"]		{
			if (*yytext == delim) {
				BEGIN(arg);
				return DELIM;
			} else {
				yylval.as_string = g_strndup(yytext, yyleng);
				return TEXT;
			}
		}

&{decimal}	{ yylval.as_string = g_strndup(yytext+1, yyleng-1); return INTERP; }
&\{{decimal}\}	{ yylval.as_string = g_strndup(yytext+2, yyleng-3); return INTERP; }
&&		|
&		{ yylval.as_string = g_strndup("&", 1); return TEXT; }

[^&/"\n\r]+	{ yylval.as_string = g_strndup(yytext, yyleng); return TEXT; }

\r		/* skip CR */
\n		{ BEGIN(INITIAL); return '\n'; }

}

%%

static int id_or_reg(void) {
	enum reg_id r = reg_name_to_id(yytext);
	if (r != REG_INVALID) {
		yylval.as_reg = r;
		return REGISTER;
	}
	yylval.as_string = g_strndup(yytext, yyleng);
	return ID;
}

/*
 * Redefining YY_INPUT above, flex reads a line at a time.  Copies of lines are
 * cached, to be fetched by the grammar parser and associated with the parsed
 * data.
 */

static GSList *lines = NULL;
static char *line = NULL;
static int line_length = 0;

static int read_line(char *buf, int max_size) {
	char *r = fgets(buf, max_size, yyin);
	if (!r) {
		if (line) {
			g_free(line);
			line = NULL;
			line_length = 0;
		}
		if (!feof(yyin)) {
			error(error_type_fatal, "scanner read failed");
		}
		return YY_NULL;
	}

	int len = strlen(buf);
	line = g_realloc(line, line_length + len + 1);
	memcpy(line + line_length, buf, len + 1);
	line_length = line_length + len;

	if (line_length > 0 && line[line_length-1] == '\n') {
		line[line_length-1] = 0;
		lines = g_slist_append(lines, line);
		line = NULL;
		line_length = 0;
	}

	return len;
}

char *lex_fetch_line(void) {
	if (!lines) {
		error(error_type_fatal, "internal: line copy fetched before ready");
		return NULL;
	}
	char *l = lines->data;
	lines = g_slist_remove(lines, l);
	return l;
}

void lex_free_all(void) {
	while (lines) {
		char *l = lines->data;
		lines = g_slist_remove(lines, l);
		g_free(l);
	}
	yylex_destroy();
}
