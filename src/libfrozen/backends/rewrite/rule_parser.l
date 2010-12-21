%{
#include "libfrozen.h"
#include "backends/rewrite/rewrite.h"
#include "backends/rewrite/rule_parser.tab.h"
%}

%option noyywrap
%option case-insensitive
%option noinput
%option nounput

request request_t
var     var_t
if      if
else    else
name    [a-z][a-z0-9_]*
string1 \'[^\n\r\']*\'
string2 \"[^\n\r\"]*\"
string  {string1}|{string2}

blank   [ \n\r\t]+

%%

{request}       { return REQUEST; }
{var}           { return VAR;     }
{if}            { return IF;      }
{else}          { return ELSE;    }
{name}          { yylval->string = strdup(yytext); return NAME;  }
{string}        {
			yytext++;                         // remove leading '"
			yytext[strlen(yytext)-1] = '\0';  // remove trailing '"
			yylval->string = strdup(yytext);
			return STRING;
		}


{blank}
.               {return *yytext;}

%%

