%{
#include <libfrozen.h>
#include <rewrite.h>	
#include <rule_parser.tab.h>	

void yyerror (rewrite_script_t *, const char *);

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE rewrite__scan_string (const char *string);  
extern int rewrite_lex_destroy(void);
extern int rewrite_lex(YYSTYPE *);

static rewrite_actions           rewrite_get_function(char *string);

static rewrite_action_block_t *  rewrite_enter_action_block(rewrite_script_t *script);
static ssize_t                   rewrite_leave_action_block(rewrite_script_t *script);
static void                      rewrite_free_action_block(rewrite_action_block_t *ablock);

static rewrite_action_t *        rewrite_new_action(rewrite_script_t *script);
static void                      rewrite_free_action(rewrite_action_t *action);

static rewrite_variable_t *      rewrite_new_constant(rewrite_script_t *script);
static void                      rewrite_free_constants(rewrite_script_t *script);

static rewrite_name_t *          rewrite_new_name(rewrite_script_t *script, char *name);
static rewrite_name_t *          rewrite_find_name(rewrite_script_t *script, char *name);
static void                      rewrite_free_names(rewrite_script_t *script);

static unsigned int              rewrite_new_variable(rewrite_script_t *script);
static unsigned int              rewrite_new_request(rewrite_script_t *script, char *req_name);
static unsigned int              rewrite_new_named_variable(rewrite_script_t *script, char *var_name);

static rewrite_thing_t *         rewrite_copy_list(rewrite_thing_t *arg1, rewrite_thing_t *arg2, rewrite_thing_t *arg3);

static rewrite_thing_t *         rewrite_copy_thing(rewrite_thing_t *thing);
static void                      rewrite_free_thing(rewrite_thing_t *thing);


%}

%start  start

%define api.pure
%parse-param {rewrite_script_t *script}

%union {
	unsigned int     value;
	char            *string;
	rewrite_thing_t  thing;
}
%token IF IFNOT ELSE
%token <value>    DIGITS
%token <string>   NAME STRING
%token REQUEST VAR
%type  <thing>    action_block
%type  <thing>    action statement define
%type  <thing>    array_key
%type  <thing>    label
%type  <thing>    constant args_list function
%type  <thing>    any_target src_target dst_target

%%

start : action_block { /* set starting block as main procedure */ script->main = $1.block; };

action_block : /* empty */ {
		/* enter new action block */
		rewrite_action_block_t *new_block = rewrite_enter_action_block(script);
		if(new_block == NULL){
			yyerror(script, "no memory\n"); YYERROR;
		}
		$<thing>$.type  = THING_ACTION_BLOCK;
		$<thing>$.block = new_block;
	}
        statements {
		/* leave block */
		if(rewrite_leave_action_block(script) < 0){
			yyerror(script, "internal error\n"); YYERROR;
		}
        }
      ;

statements: /* empty */
	| statements statement ';'
	;

statement: 
	  '(' statement ')' { $$ = $2; }
	| define
	| action
	;

define :
       REQUEST NAME { rewrite_new_request(script, $2);        free($2); }
     | VAR NAME     { rewrite_new_named_variable(script, $2); free($2); }
     ;

action :
	dst_target '=' any_target /* set or unset */ {
		/* make new action */
		rewrite_action_t *action = rewrite_new_action(script);
		action->action  = LANG_SET;
		action->params  = rewrite_copy_list(&$1, &$3, NULL);
	}
     | function
     | any_target
     | IF statement '{' action_block '}' {
		/* make new action */
		rewrite_action_t *action = rewrite_new_action(script);
		action->action  = LANG_IF;
		action->params  = rewrite_copy_list(&$2, &$4, NULL);
     }
     | IFNOT statement '{' action_block '}' {
		/* make new action */
		rewrite_action_t *action = rewrite_new_action(script);
		action->action  = LANG_IFNOT;
		action->params  = rewrite_copy_list(&$2, &$4, NULL);
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
	data_t              d_src    = DATA_PTR_STRING($4);
	rewrite_variable_t *constant = rewrite_new_constant(script);
	
	constant->data.type = data_type_from_string($2);
	constant->data.ptr  = NULL;
	
	/* convert string to needed data */
	fastcall_convert r_convert = { { 3, ACTION_CONVERT }, &d_src }; 
	if(data_query(&constant->data, &r_convert) != 0){
		yyerror(script, "failed convert data\n"); YYERROR;
	}

	/* fill output type */
	$$.type = THING_CONST;
	$$.id   = constant->id;
	
	free($2);
	free($4);
};

array_key : NAME '[' STRING ']' {
	rewrite_name_t *curr;
	if((curr = rewrite_find_name(script, $1)) != NULL && curr->type == THING_HASHT){
		$$.type      = THING_HASH_ELEMENT;
		$$.array_key = hash_string_to_key($3);
		$$.id        = curr->id;
		
		free($1);
		free($3);
		break;
	}
	
	yyerror(script, "invalid target array\n"); YYERROR;
};

label : NAME {
	rewrite_name_t *curr;
	if((curr = rewrite_find_name(script, $1)) != NULL){
		$$.type = curr->type;
		$$.id   = curr->id;
		
		free($1);
		break;
	}
	
	data_functions action;
	if((action = request_str_to_action($1)) != ACTION_INVALID){
		rewrite_variable_t *constant = rewrite_new_constant(script);
		data_t              d_act    = DATA_UINT32T(action);
		
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &constant->data };
		data_query(&d_act, &r_copy);
		
		/* fill output type */
		$$.type = THING_CONST;
		$$.id   = constant->id;
		
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
			$1.next = NULL;
			
			/* fill output type */
			$$.type = THING_LIST;
			$$.list = rewrite_copy_thing(&$1);
			$$.next = NULL;
		}
	| args_list ',' any_target {
			$3.next = NULL;
			
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
	if(strcasecmp(string, "query")          == 0) return BACKEND_QUERY;
	if(strcasecmp(string, "pass")           == 0) return BACKEND_PASS;
	if(strcasecmp(string, "data_query")     == 0) return DATA_QUERY;
#ifdef DEBUG
	if(strcasecmp(string, "hash_dump")      == 0) return HASH_DUMP;
#endif
	return DO_NOTHING;
}

/* action blocks {{{ */
static rewrite_action_block_t * rewrite_enter_action_block(rewrite_script_t *script){ // {{{
	rewrite_action_block_t *ablock;
	
	if( (ablock = malloc(sizeof(rewrite_action_block_t))) == NULL)
		return NULL;
	
	ablock->actions       = NULL;
	ablock->actions_count = 0;
	ablock->next          = script->main;
	script->main          = ablock;
	
	return ablock;
} // }}}
static ssize_t              rewrite_leave_action_block(rewrite_script_t *script){ // {{{
	rewrite_action_block_t *leaved;
	
	if( (leaved = script->main) == NULL)
		return -1;
	
	script->main = leaved->next;
	return 0;
} // }}}
static void                 rewrite_free_action_block(rewrite_action_block_t *ablock){ // {{{
	unsigned int id;
	
	for(id=0; id < ablock->actions_count; id++)
		rewrite_free_action( &ablock->actions[id] );
	free(ablock->actions);
	free(ablock);
} // }}}
/* }}} */
/* action {{{ */
static rewrite_action_t *   rewrite_new_action(rewrite_script_t *script){ // {{{
	unsigned int id = script->main->actions_count++;
	script->main->actions = realloc(script->main->actions, script->main->actions_count * sizeof(rewrite_action_t));
	memset(&script->main->actions[id], 0, sizeof(rewrite_action_t));
	script->main->actions[id].id = id;
	
	return &script->main->actions[id];
} // }}}
static void                 rewrite_free_action(rewrite_action_t *action){ // {{{
	rewrite_free_thing(action->params);
	rewrite_free_thing(action->ret);
} // }}}
/* }}} */
/* constants {{{ */
static rewrite_variable_t * rewrite_new_constant(rewrite_script_t *script){ // {{{
	unsigned int id = script->constants_count++;
	script->constants = realloc(script->constants, script->constants_count * sizeof(rewrite_variable_t));
	memset(&script->constants[id], 0, sizeof(rewrite_variable_t));
	script->constants[id].id = id;
	
	return &script->constants[id];
} // }}}
static void                 rewrite_free_constants(rewrite_script_t *script){ // {{{
	unsigned int i;
	
	for(i=0; i<script->constants_count; i++){
		rewrite_variable_t *constant = &script->constants[i];
		
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(&constant->data, &r_free);
	}
	free(script->constants);
} // }}}
/* }}} */
/* variables {{{ */
static unsigned int         rewrite_new_variable(rewrite_script_t *script){ // {{{
	return script->variables_count++;
} // }}}
static unsigned int         rewrite_new_named_variable (rewrite_script_t *script, char *var_name){ // {{{
	rewrite_name_t *name = rewrite_new_name(script, var_name);
	name->type = THING_VARIABLE;
	name->id   = script->variables_count++;
	return name->id;
} // }}}
/* }}} */
/* requests {{{ */
static unsigned int         rewrite_new_request (rewrite_script_t *script, char *req_name){ // {{{
	rewrite_name_t *name = rewrite_new_name(script, req_name);
	name->type = THING_HASHT;
	name->id   = script->requests_count++;
	return name->id;
} // }}}
/* }}} */

/* names {{{ */
static rewrite_name_t *     rewrite_new_name(rewrite_script_t *script, char *name){ // {{{
	rewrite_name_t *tname = malloc(sizeof(rewrite_name_t));
	tname->name = strdup(name);
	
	tname->next   = script->names;
	script->names = tname;
	
	return tname;
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
static void                 rewrite_free_names(rewrite_script_t *script){ // {{{
	rewrite_name_t *curr, *next = script->names;
	while((curr = next) != NULL){
		next = curr->next;
		
		free(curr->name);
		free(curr);
	}
} // }}}
/* }}} */

static rewrite_thing_t *    rewrite_copy_list(rewrite_thing_t *arg1, rewrite_thing_t *arg2, rewrite_thing_t *arg3){ // {{{
	rewrite_thing_t  set_params;
	
	arg1->next = arg2;
	if(arg2 != NULL)
		arg2->next = arg3;
	if(arg3 != NULL)
		arg3->next = NULL;
	
	/* set_params = (list)(arg1, arg2) */
	set_params.type = THING_LIST;
	set_params.list = rewrite_copy_thing(arg1);
	set_params.next = NULL;
	return rewrite_copy_thing(&set_params);
}
/*
static rewrite_thing_t *    rewrite_copy_list3(rewrite_thing_t *arg1, rewrite_thing_t *arg2, rewrite_thing_t *arg3){ // {{{
	rewrite_thing_t  set_params;
	
	arg1->next = arg2;
	arg2->next = arg3;
	arg3->next = NULL;
	
	// set_params = (list)(arg1, arg2, arg3) 
	set_params.type = THING_LIST;
	set_params.list = rewrite_copy_thing(arg1);
	set_params.next = NULL;
	return rewrite_copy_thing(&set_params);
}*/

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
	
	if(thing->next)
		rewrite_free_thing(thing->next);
	if(thing->type == THING_LIST)
		rewrite_free_thing(thing->list);
	if(thing->type == THING_ACTION_BLOCK)
		rewrite_free_action_block(thing->block);
	
	free(thing);
} // }}}

void yyerror(rewrite_script_t *script, const char *msg){
	fprintf(stderr, "rewrite rule error: '%s': %s\n",
		script->script,
		msg
	);
}

ssize_t  rewrite_script_parse(rewrite_script_t *script, char *string){ // {{{
	ssize_t          ret;
	rewrite_name_t  *name;
	
	memset(script, 0, sizeof(rewrite_script_t));
	script->script = string;
	
	/* add special keywords */
	rewrite_new_request(script, "request"); // id=0
	
	name = rewrite_new_name(script, "ret");
	name->type = THING_RET;
	
	rewrite__scan_string(string);
	switch(yyparse(script)){
		case 0:  ret = 0;       goto exit;
		case 1:  ret = -EINVAL; break;
		case 2:  ret = -ENOMEM; break;
		default: ret = -EFAULT; break;
	};
	rewrite_script_free(script);
exit:
	rewrite_lex_destroy();
	return ret;
} // }}}
void rewrite_script_free(rewrite_script_t *script){ // {{{
	rewrite_free_names(script);
	rewrite_free_constants(script);
	rewrite_free_action_block(script->main);
} // }}}

