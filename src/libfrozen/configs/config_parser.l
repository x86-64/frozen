%{
#include "libfrozen.h"
#include "configs/config_parser.tab.h"
%}

%option noyywrap
%option case-insensitive
%option noinput
%option nounput

name    [a-z][a-z0-9_]*
string1 \'[^\']*\'
string2 \"[^\"]*\"
string  {string1}|{string2}
assign1 :
assign2 =>?
assign  {assign1}|{assign2}
null    NULL

blank   [ \n\r\t]+

%%

{null}          { return TNULL; }
{assign}        { return ASSIGN; }
{name}          { yylval->name = strdup(yytext); return NAME;  }
{string}        {
			yytext++;                         // remove leading '"
			yytext[strlen(yytext)-1] = '\0';  // remove trailing '"
			yylval->name = strdup(yytext);
			return STRING;
		}

{blank}
.               { return *yytext; }

%%

