%{
#include <libfrozen.h>
#include <backends/rewrite/rewrite.h>	
#include <backends/rewrite/rule_parser.tab.h>	

typedef struct yycontext_t {
	rewrite_rule_t     *rule;
	char               *string;
	ssize_t             ret;
} yycontext_t;

void yyerror (yycontext_t *, const char *);

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string (const char *string);  
extern int yylex_destroy(void);
extern int yylex(YYSTYPE *);

%}

%start  statement

%define api.pure
%parse-param {yycontext_t *ctx}

%union {
	int       value;
	char     *string;
	target_t  target;
}
%token <value>  DIGITS
%token <string> NAME TYPE STRING
%type  <target> target

%%

statement :
	target '=' target /* set or unset */
		{
			ctx->rule->action = VALUE_SET;
			memcpy(&ctx->rule->dst, &$1, sizeof($1));
			memcpy(&ctx->rule->src, &$3, sizeof($3));
		}
     ;

target :  NAME
		{
			if(strcmp($1, "VOID") == 0){
				$$.type = THING_NOTHING;
			}else{
				yyerror(ctx, "invalid target\n");
				YYERROR;
			}
		}
	| NAME '[' STRING ']'
		{
			if(strcmp($1, "request") == 0){
				$$.type     = THING_REQUEST;
				$$.rule_num = 0;
			}else{
				yyerror(ctx, "invalid target array\n");
				YYERROR;
			}
			$$.subkey   = $3;
		}
	| '(' TYPE ')' STRING
		{
			$$.type = THING_CONFIG;
			
			data_t d_str = DATA_PTR_STRING($4, strlen($4)+1);
			
			data_reinit(&$$.data, data_type_from_string($2), NULL, 0);
			if(data_convert(
				&$$.data, NULL,
				&d_str, NULL
			) != 0){
				yyerror(ctx, "failed convert data\n");
				YYERROR;
			}
		}
	;

exp_list : exp
         | exp ',' exp_list
	 ;

exp : target
    ;

%%

void yyerror(yycontext_t *ctx, const char *msg){
	error("rewrite rule error: '%s': %s\n",
		ctx->string,
		msg
	);
	ctx->ret = -EINVAL;
}

ssize_t  rewrite_rule_parse(rewrite_rule_t *new_rule, char *string){ // {{{
	ssize_t      ret;
	yycontext_t  ctx;
	
	ctx.rule       = new_rule;
	ctx.string     = string;
	ctx.ret        = 0;
	
	memset(new_rule, 0, sizeof(rewrite_rule_t));
	new_rule->filter = REQUEST_INVALID;
	new_rule->time   = TIME_BEFORE;
	new_rule->action = DO_NOTHING;
	
	yy_scan_string(string);
	switch(yyparse(&ctx)){
		case 0:  ret = 0;       goto exit;
		case 1:  ret = ctx.ret; break;
		case 2:  ret = -ENOMEM; break;
		default: ret = -EFAULT; break;
	};
	rewrite_rule_free(new_rule);
exit:
	yylex_destroy();
	return ret;
} // }}}

void rewrite_rule_free(rewrite_rule_t *rule){ // {{{
	/*
	if(rule->src_key != NULL) free(rule->src_key);
	if(rule->dst_key != NULL) free(rule->dst_key);
	
	data_free(&rule->src_config);
	
	hash_free(rule->request_proto);
	
	if(rule->backend != NULL)
		backend_destroy(rule->backend);
	*/
} // }}}

