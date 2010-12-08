%{
#include "libfrozen.h"
#include "backends/rewrite/rewrite.h"
#include "backends/rewrite/rule_parser.tab.h"
%}

%option noyywrap
%option case-insensitive
%option noinput
%option nounput

name    [a-z][a-z0-9]*
type    [a-z][a-z0-9_]*
string1 \'[^\n\r\']*\'
string2 \"[^\N\R\"]*\"
string  {string1}|{string2}


blank   [ \n\r\t]+

%%

{name}          { yylval->string = strdup(yytext); return NAME;  }
{type}          { yylval->string = strdup(yytext); return TYPE;  }
{string}        {
			yytext++;                         // remove leading '"
			yytext[strlen(yytext)-1] = '\0';  // remove trailing '"
			yylval->string = strdup(yytext);
			return STRING;
		}


{blank}
.               {return *yytext;}

%%

