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
static rewrite_name_t *     rewrite_find_name(rewrite_script_t *script, char *name);
static unsigned int         rewrite_new_variable(rewrite_script_t *script);
static unsigned int         rewrite_new_request(rewrite_script_t *script, char *req_name);
static unsigned int         rewrite_new_named_variable(rewrite_script_t *script, char *var_name);

static rewrite_thing_t *    rewrite_copy_thing(rewrite_thing_t *thing);
static void                 rewrite_free_thing(rewrite_thing_t *thing);


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
%token REQUEST VAR
%type  <thing>    array_key
%type  <thing>    label
%type  <thing>    constant args_list function
%type  <thing>    any_target src_target dst_target

%%

input : /* empty */
      | input statement ';'
      ;

statement :
	dst_target '=' any_target /* set or unset */ {
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
     | REQUEST NAME {
		rewrite_new_request(script, $2);
		free($2);
     	}
     | VAR NAME {
     		rewrite_new_named_variable(script, $2);
		free($2);
	}
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

	free($1);
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
	
	free($2);
	free($4);
};

array_key : NAME '[' STRING ']' {
	if(strcmp($1, "request") == 0){
		/* fill output type */
		$$.type      = THING_ARRAY_REQUEST_KEY;
		$$.array_key = $3;
		$$.id        = 0;
		$$.next      = NULL;
		
		free($1);
		break;
	}
	
	rewrite_name_t *curr;
	if((curr = rewrite_find_name(script, $1)) != NULL && curr->type == THING_ARRAY_REQUEST){
		$$.type      = THING_ARRAY_REQUEST_KEY;
		$$.array_key = $3;
		$$.id        = curr->id;
		$$.next      = NULL;
		
		free($1);
		break;
	}
	
	yyerror(script, "invalid target array\n"); YYERROR;
};

label : NAME {
	if(strcmp($1, "request") == 0){
		$$.type = THING_ARRAY_REQUEST;
		$$.id   = 0;
		$$.next = NULL;
		
		free($1);
		break;
	}
	if(strcmp($1, "ret") == 0){
		$$.type = THING_RET;
		$$.next = NULL;
		
		free($1);
		break;
	}
	
	rewrite_name_t *curr;
	if((curr = rewrite_find_name(script, $1)) != NULL){
		$$.type = curr->type;
		$$.id   = curr->id;
		$$.next = NULL;
		
		free($1);
		break;
	}
	
	request_actions action;
	if((action = request_str_to_action($1)) != REQUEST_INVALID){
		rewrite_variable_t *constant = rewrite_new_constant(script);
		data_t              d_act    = DATA_INT32(action);
		
		data_copy(&constant->data, &d_act);
		
		/* fill output type */
		$$.type = THING_CONST;
		$$.id   = constant->id;
		$$.next = NULL;
		
		free($1);
		break;
	}
	
	yyerror(script, "invalid target array\n"); YYERROR;
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
static void                 rewrite_free_action(rewrite_script_t *script, unsigned int id){ // {{{
	rewrite_action_t *action = &script->actions[id];
	
	rewrite_free_thing(action->params);
	rewrite_free_thing(action->ret);
} // }}}
static rewrite_variable_t * rewrite_new_constant(rewrite_script_t *script){ // {{{
	unsigned int id = script->constants_count++;
	script->constants = realloc(script->constants, script->constants_count * sizeof(rewrite_variable_t));
	memset(&script->constants[id], 0, sizeof(rewrite_variable_t));
	script->constants[id].id = id;
	
	return &script->constants[id];
} // }}}
static void                 rewrite_free_constant(rewrite_script_t *script, unsigned int id){ // {{{
	rewrite_variable_t *constant = &script->constants[id];
	
	data_free(&constant->data);
} // }}}

static unsigned int         rewrite_new_variable(rewrite_script_t *script){ // {{{
	return script->variables_count++;
} // }}}
static unsigned int         rewrite_new_named_variable (rewrite_script_t *script, char *var_name){ // {{{
	unsigned int id = script->variables_count++;
	rewrite_name_t *name = malloc(sizeof(rewrite_name_t));
	name->type = THING_VARIABLE;
	name->id   = id;
	name->name = strdup(var_name);
	
	name->next = script->names;
	script->names = name;
	
	return id;
} // }}}
static unsigned int         rewrite_new_request (rewrite_script_t *script, char *req_name){ // {{{
	unsigned int id = script->requests_count++;
	rewrite_name_t *name = malloc(sizeof(rewrite_name_t));
	name->type = THING_ARRAY_REQUEST;
	name->id   = id;
	name->name = strdup(req_name);
	
	name->next = script->names;
	script->names = name;
	
	return id;
} // }}}
static rewrite_name_t *     rewrite_find_name(rewrite_script_t *script, char *name){ // {{{
	rewrite_name_t *curr = script->names;
	while(curr != NULL){
		if(strcmp(curr->name, name) == 0)
			return curr;
		
		curr = curr->next;
	}
	return NULL;
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
static void                 rewrite_free_thing(rewrite_thing_t *thing){ // {{{
	if(thing == NULL)
		return;
	
	if(thing->type == THING_ARRAY_REQUEST_KEY)
		free(thing->array_key);
	
	if(thing->next)
		rewrite_free_thing(thing->next);
	if(thing->type == THING_LIST)
		rewrite_free_thing(thing->list);
	
	free(thing);
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
	script->script         = string;
	script->requests_count = 1; // one initial request
	
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
	unsigned int i;
	
	rewrite_name_t *curr, *next = script->names;
	while((curr = next) != NULL){
		next = curr->next;
		
		free(curr->name);
		free(curr);
	}
	
	for(i=0; i<script->constants_count; i++)
		rewrite_free_constant(script, i);
	free(script->constants);
	
	for(i=0; i<script->actions_count; i++)
		rewrite_free_action(script, i);
	free(script->actions);
} // }}}

