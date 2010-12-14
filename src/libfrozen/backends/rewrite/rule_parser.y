%{
#include <libfrozen.h>
#include <backends/rewrite/rewrite.h>	
#include <backends/rewrite/rule_parser.tab.h>	

void yyerror (rewrite_script_t *, const char *);

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string (const char *string);  
extern int yylex_destroy(void);
extern int yylex(YYSTYPE *);

static rewrite_actions      rewrite_get_function(char *string);
static rewrite_action_t *   rewrite_new_action(rewrite_script_t *script);
static rewrite_variable_t * rewrite_new_constant(rewrite_script_t *script);
static unsigned int         rewrite_new_variable(rewrite_script_t *script);
static rewrite_thing_t *    rewrite_copy_thing(rewrite_thing_t *thing);

%}

%start  input

%define api.pure
%parse-param {rewrite_script_t *script}

%union {
	unsigned int     value;
	char            *string;
	rewrite_thing_t  thing;
}
%token <value>    DIGITS
%token <string>   NAME STRING
%type  <thing>    array_key
%type  <thing>    label
%type  <thing>    constant args_list function
%type  <thing>    any_target src_target dst_target

%destructor { free($$); } <string>

%%

input : /* empty */
      | input statement ';'
      ;

statement :
	dst_target '=' any_target /* set or unset */
		{
			rewrite_thing_t  set_params;
			
			$1.next = &$3;
			/* set_params = (list)(dst_target, any_target) */
			set_params.type = THING_LIST;
			set_params.list = rewrite_copy_thing(&$1);
			set_params.next = NULL;

			/* make new action */
			rewrite_action_t *action = rewrite_new_action(script);
			action->action     = VALUE_SET;
			action->params     = rewrite_copy_thing(&set_params);
		}
     | function
     ;

dst_target : array_key | label;
src_target : constant | function | label;
any_target : src_target | dst_target;

function : NAME '(' args_list ')' {
	rewrite_actions func;
	if( (func = rewrite_get_function($1)) == DO_NOTHING){
		yyerror(script, "unknown function\n"); YYERROR;
	}
	
	/* fill output type */
	$$.type = THING_VARIABLE;
	$$.id   = rewrite_new_variable(script);
	$$.next = NULL;
	
	/* make new action */
	rewrite_action_t *action = rewrite_new_action(script);
	action->action     = func;
	action->params     = rewrite_copy_thing(&$3);
	action->ret        = rewrite_copy_thing(&$$);
};

constant : '(' NAME ')' STRING {
	rewrite_variable_t *constant = rewrite_new_constant(script);
	data_t              d_str    = DATA_PTR_STRING($4, strlen($4)+1);
	
	/* convert string to needed data */
	data_reinit(&constant->data, data_type_from_string($2), NULL, 0);
	if(data_convert(
		&constant->data, NULL,
		&d_str,          NULL
	) != 0){
		yyerror(script, "failed convert data\n"); YYERROR;
	}
	
	/* fill output type */
	$$.type = THING_CONST;
	$$.id   = constant->id;
	$$.next = NULL;
};

array_key : NAME '[' STRING ']' {
	if(strcmp($1, "request") == 0){
		/* fill output type */
		$$.type      = THING_ARRAY_REQUEST_KEY;
		$$.array_key = $3;
		$$.id        = 0;
		$$.next      = NULL;
	}else{
		yyerror(script, "invalid target array\n"); YYERROR;
	}
};

label : NAME {
	if(strcmp($1, "request") == 0){
		$$.type      = THING_ARRAY_REQUEST;
		$$.id        = 0;
	}else if(strcmp($1, "ret") == 0){
		$$.type      = THING_RET;
	}else{
		yyerror(script, "invalid target array\n"); YYERROR;
	}
	$$.next = NULL;
};

args_list : /* empty args */ {
			$$.type = THING_LIST;
			$$.list = NULL;
			$$.next = NULL;
		}
	| any_target {
			/* fill output type */
			$$.type = THING_LIST;
			$$.list = rewrite_copy_thing(&$1);
			$$.next = NULL;
		}
	| args_list ',' any_target {
			/* append item to args_list */
			rewrite_thing_t *last = $1.list, *new_thing = rewrite_copy_thing(&$3);
			if(last != NULL){
				while(last->next != NULL){ last = last->next; }
				last->next = new_thing;
			}else{
				last->list = new_thing;
			}
			
			$$.type = THING_LIST;
			$$.list = $1.list;
			$$.next = NULL;
		};

%%

static rewrite_actions   rewrite_get_function(char *string){
	if(strcasecmp(string, "length")         == 0) return VALUE_LENGTH;
	if(strcasecmp(string, "backend")        == 0) return CALL_BACKEND;
	if(strcasecmp(string, "pass")           == 0) return CALL_PASS;
	if(strcasecmp(string, "data_length")    == 0) return DATA_LENGTH;
	if(strcasecmp(string, "data_arith")     == 0) return DATA_ARITH;
	if(strcasecmp(string, "data_convert")   == 0) return DATA_CONVERT;
	if(strcasecmp(string, "data_alloca")    == 0) return DATA_ALLOCA;
	if(strcasecmp(string, "data_free")      == 0) return DATA_FREE;
#ifdef DEBUG
	if(strcasecmp(string, "hash_dump")      == 0) return HASH_DUMP;
#endif
	return DO_NOTHING;
}

static rewrite_action_t *   rewrite_new_action(rewrite_script_t *script){ // {{{
	unsigned int id = script->actions_count++;
	script->actions = realloc(script->actions, script->actions_count * sizeof(rewrite_action_t));
	memset(&script->actions[id], 0, sizeof(rewrite_action_t));
	script->actions[id].id = id;
	
	return &script->actions[id];
} // }}}
static rewrite_variable_t * rewrite_new_constant(rewrite_script_t *script){ // {{{
	unsigned int id = script->constants_count++;
	script->constants = realloc(script->constants, script->constants_count * sizeof(rewrite_variable_t));
	memset(&script->constants[id], 0, sizeof(rewrite_variable_t));
	script->constants[id].id = id;
	
	return &script->constants[id];
} // }}}
static unsigned int         rewrite_new_variable(rewrite_script_t *script){ // {{{
	return script->variables_count++;
} // }}}
static rewrite_thing_t *    rewrite_copy_thing(rewrite_thing_t *thing){ // {{{
	rewrite_thing_t *new_thing;
	
	if(thing == NULL)
		return NULL;
	
	new_thing = malloc(sizeof(rewrite_thing_t));
	memcpy(new_thing, thing, sizeof(rewrite_thing_t));
	
	new_thing->next = rewrite_copy_thing(thing->next);
	
	return new_thing;
} // }}}

void yyerror(rewrite_script_t *script, const char *msg){
	error("rewrite rule error: '%s': %s\n",
		script->script,
		msg
	);
}

ssize_t  rewrite_script_parse(rewrite_script_t *script, char *string){ // {{{
	ssize_t      ret;
	
	memset(script, 0, sizeof(rewrite_script_t));
	script->script = string;
	
	yy_scan_string(string);
	switch(yyparse(script)){
		case 0:  ret = 0;       goto exit;
		case 1:  ret = -EINVAL; break;
		case 2:  ret = -ENOMEM; break;
		default: ret = -EFAULT; break;
	};
	rewrite_script_free(script);
exit:
	yylex_destroy();
	return ret;
} // }}}

void rewrite_script_free(rewrite_script_t *script){ // {{{
	/*
	if(rule->src_key != NULL) free(rule->src_key);
	if(rule->dst_key != NULL) free(rule->dst_key);
	
	data_free(&rule->src_config);
	
	hash_free(rule->request_proto);
	
	if(rule->backend != NULL)
		backend_destroy(rule->backend);
	*/
} // }}}

