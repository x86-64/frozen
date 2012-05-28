%{
#include <libfrozen.h>
#include <configs/config.h>
#include <configs/config_parser.tab.h>	

static void yyerror (hash_t **, const char *);

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE config__scan_string (const char *string);  
extern int config_lex_destroy(void);
extern int config_lex(YYSTYPE *);
extern int config_get_lineno(void);
extern char *config_get_text(void);

#define emit_error(fmt, ...){                                               \
	do {                                                                \
		char _buffer[DEF_BUFFER_SIZE];                              \
		                                                            \
		snprintf(_buffer, sizeof(_buffer) - 1, fmt, ##__VA_ARGS__); \
		_buffer[sizeof(_buffer) - 1] = '\0';                        \
		yyerror(hash, _buffer);                                     \
		YYERROR;                                                    \
	}while(0); }

%}

%start  start

%define api.pure
%parse-param {hash_t **hash}

%union {
	hash_t     *hash;
	hash_t      hash_item;
	hashkey_t   key;
	char       *name;
	data_t      data;
	machine_t  *pipeline;
}
%token NAME STRING ASSIGN TNULL
%type  <hash>        hash_items entity_pipelines entity_pipeline_config
%type  <hash_item>   hash_item  entity_pipeline
%type  <key>         hash_name
%type  <name>        NAME STRING
%type  <data>        hash_value
%type  <pipeline>    entity_pipeline_raw

%%

start : entity_pipelines { *hash = $1; }

/* entity: pipeline {{{*/
entity_pipelines :
	/* empty */       { $$ = hash_new(1); }
	| entity_pipeline { $$ = hash_new(2); hash_assign_hash_t(&$$[0], &$1); }
	| entity_pipelines entity_pipeline { $$ = hash_append($1, $2);  }
	;

entity_pipeline :
	entity_pipeline_raw {
		$$.key       = HK(machine);
		$$.data.type = TYPE_MACHINET;
		$$.data.ptr  = pipeline_finalize(&$1);
	}
	;

entity_pipeline_raw :
	entity_pipeline_config {
		pipeline_new(&$$);
		pipeline_append(&$$, $1);
		hash_free($1);
	}
	| entity_pipeline_raw ',' entity_pipeline_config {
		$$ = $1;
		pipeline_append(&$$, $3);
		hash_free($3);
	}
	;

entity_pipeline_config :
	'{' hash_items '}' { $$ = $2; };
/* }}} */

hash_items :
	/* empty */ { $$ = hash_new(1); }
	| hash_item { $$ = hash_new(2); hash_assign_hash_t(&$$[0], &$1); }
	| hash_items ',' hash_item { $$ = hash_append($1, $3); };

hash_item : hash_name hash_value {
		$$.key = $1;
		$$.data = $2;
	}
	| TNULL {
		$$.key = hash_ptr_null;
	}
;

hash_name :
          /* empty */  { $$ = 0; }
	| TNULL ASSIGN { $$ = 0; }
	| NAME  ASSIGN {
		ssize_t                ret;
		data_t                 d_key             = DATA_PTR_HASHKEYT(&$$);
		data_t                 d_initstr         = DATA_PTR_STRING($1);

		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_key, &r_convert)) < 0)
			emit_error("unknown hashkey_t \"%s\" (ret: %s)", $1, errors_describe(ret));
		
		free($1);
	};

hash_value :
	  STRING             {
		ssize_t                ret;
		data_t                 d_initstr    = DATA_PTR_STRING($1);
		
		data_raw_t_empty(&$$);
		
		fastcall_convert_from r_init_str = { { 5, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(config) }; 
		if( (ret = data_query(&$$, &r_init_str)) < 0)
			emit_error("data string init failed (ret: %s)", errors_describe(ret));

		free($1);
	}
	| '{' hash_items '}' { $$.type = TYPE_HASHT;   $$.ptr = $2; }
	| '(' NAME ')' STRING {
		ssize_t                ret;
		datatype_t             type;
		data_t                 d_type            = DATA_PTR_DATATYPET(&type);
		data_t                 d_type_initstr    = DATA_PTR_STRING($2);
		data_t                 d_val_initstr     = DATA_PTR_STRING($4);
		
		fastcall_convert_from r_init1 = { { 5, ACTION_CONVERT_FROM }, &d_type_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_type, &r_init1)) < 0)
			emit_error("unknown datatype_t \"%s\" (ret: %s)", $2, errors_describe(ret));
		
		$$.type = type;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_init2 = { { 5, ACTION_CONVERT_FROM }, &d_val_initstr, FORMAT(config) }; 
		if( (ret = data_query(&$$, &r_init2)) < 0)
			emit_error("data init failed \"%s\" (ret: %s)", $2, errors_describe(ret));
		
		free($2);
		free($4);
	}
	| '(' NAME ')' '{' hash_items '}' {
		ssize_t                ret;
		datatype_t             type;
		data_t                 d_type            = DATA_PTR_DATATYPET(&type);
		data_t                 d_type_initstr    = DATA_PTR_STRING($2);
		data_t                 d_hash            = DATA_PTR_HASHT($5);

		fastcall_convert_from r_init1 = { { 5, ACTION_CONVERT_FROM }, &d_type_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_type, &r_init1)) < 0)
			emit_error("unknown datatype_t \"%s\" (ret: %s)", $2, errors_describe(ret));
		
		$$.type = type;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_hash, FORMAT(hash) }; 
		if( (ret = data_query(&$$, &r_convert)) < 0)
			emit_error("data init failed \"%s\" (ret: %s)", $2, errors_describe(ret));
		
		free($2);
		hash_free($5);
	};

%%

static void yyerror(hash_t **hash, const char *msg){ // {{{
	char                  *file              = config_ext_file; 
	
	if(!file)
		file = "-";
	
	fprintf(stderr, "%s: error: %d: %s\n", file, config_get_lineno(), msg);
	(void)hash;
} // }}}

hash_t *   configs_pl_string_parse(char *string){ // {{{
	hash_t *new_hash = NULL;
	
	config__scan_string(string);
	
	yyparse(&new_hash);
	
	config_lex_destroy();
	return new_hash;
} // }}}
