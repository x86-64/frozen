%{
#include "libfrozen.h"
#include "configs/config_parser.tab.h"

#define NEW_STRING_STEP 100

char        *new_string_buf;
char        *new_string;
char        *new_string_end;
char         new_string_terminator;

void check_new_string(uintmax_t size);

%}

%option noyywrap
%option case-insensitive
%option noinput
%option nounput

%x comment
%x str

name    [a-z][a-z0-9_]*
assign1 :
assign2 =>?
assign  {assign1}|{assign2}
null    NULL

blank   [ \n\r\t]+

%%

"//"+[^\n]*     { }
"/*"                    BEGIN(comment);
<comment>[^*]*
<comment>"*"+[^*/]*
<comment>"*"+"/"        BEGIN(INITIAL);



\"           new_string_buf = new_string = malloc(NEW_STRING_STEP); new_string_end = new_string_buf + NEW_STRING_STEP; new_string_terminator = '"';  BEGIN(str);
\'           new_string_buf = new_string = malloc(NEW_STRING_STEP); new_string_end = new_string_buf + NEW_STRING_STEP; new_string_terminator = '\''; BEGIN(str);
#            new_string_buf = new_string = malloc(NEW_STRING_STEP); new_string_end = new_string_buf + NEW_STRING_STEP; new_string_terminator = '#';  BEGIN(str);
@            new_string_buf = new_string = malloc(NEW_STRING_STEP); new_string_end = new_string_buf + NEW_STRING_STEP; new_string_terminator = '@';  BEGIN(str);
<str>['"#@]  {
	if(*yytext == new_string_terminator){
		check_new_string(1);
		*new_string = '\0';
		
		BEGIN(INITIAL);
		yylval->name = new_string_buf;
		return STRING;
	}
	check_new_string(1);
	*new_string++ = yytext[0];
}
<str>\\n { check_new_string(1); *new_string++ = '\n';      }
<str>\\r { check_new_string(1); *new_string++ = '\r';      }
<str>\\t { check_new_string(1); *new_string++ = '\t';      }
<str>\\. { check_new_string(1); *new_string++ = yytext[1]; }
<str>[^\\\n\"\'#@]+ {
	uintmax_t   size = strlen(yytext);
	check_new_string(size);
	
	memcpy(new_string, yytext, size);
	new_string += size;
}

{null}          { return TNULL; }
{assign}        { return ASSIGN; }
{name}          { yylval->name = strdup(yytext); return NAME;  }

{blank}
.               { return *yytext; }

%%

void check_new_string(uintmax_t size){
	uintmax_t  new_string_size, new_string_ptr;
	
	if(new_string + size >= new_string_end){
		new_string_ptr  = new_string     - new_string_buf;
		new_string_size = new_string_end - new_string_buf;
		
		if( (new_string_buf  = realloc(new_string_buf, new_string_size + size + NEW_STRING_STEP)) == NULL){
			fprintf(stderr, "string too long\n");
			exit(255); // TODO fix it
		}
		new_string      = new_string_buf + new_string_ptr;
		new_string_end  = new_string_buf + new_string_size + size + NEW_STRING_STEP; 
	}
}
