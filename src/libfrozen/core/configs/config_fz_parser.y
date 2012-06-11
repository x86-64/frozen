%{
//#define YYDEBUG 1

#include <libfrozen.h>
#include <configs/config.h>
#include <configs/config_fz_parser.tab.h>	

typedef enum fz_keyword {
	KEYWORD_LOCAL  = 1,
	KEYWORD_GLOBAL = 2,
} fz_keyword;

keypair_t fz_keywords[] = {
	{ "local",  KEYWORD_LOCAL  },
	{ "global", KEYWORD_GLOBAL },
	{ NULL, 0 }
};

static void          yyerror(hash_t **, const char *);
static ssize_t       config_fz_keyword(uintmax_t *keyword, char *string);
//static ssize_t       config_fz_keyword_validate(uintmax_t *pkeyword);

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

#define config_fz_lex config_lex

%}

%start  start

%define api.pure
%parse-param {hash_t **hash}

%union {
	hash_t     *hash_items;
	hash_t      hash_item;
	
	char       *name;
	data_t      data;
	
	machine_t  *pipeline;
	hash_t     *pipeline_config;

	action_t    action;
	hashkey_t   hashkey;
	datatype_t  datatype;
	uintmax_t   keyword;
}
%token NAME STRING ASSIGN TNULL KEYWORD SUB
%nonassoc '~' ':' '>'

%type  <name>            NAME STRING KEYWORD

%type  <hash_items>      statements
%type  <hash_item>       statement
%type  <hash_items>      hash_items
%type  <hash_item>       hash_item

%type  <data>            data
%type  <data>            data_simple
%type  <data>            data_complex
%type  <data>            data_complex_rules
%type  <data>            data_converted

%type  <data>            statement_named_data
%type  <data>            pipeline

%type  <keyword>         keyword

%type  <action>          action_t
%type  <datatype>        datatype_t
%type  <hashkey>         env_t_key
%type  <hashkey>         hashkey_t

%type  <pipeline>        pipeline_raw
%type  <pipeline_config> pipeline_machine
%type  <pipeline_config> pipeline_machine_enum
%type  <pipeline_config> pipeline_machine_query
%type  <pipeline_config> pipeline_machine_assign
%type  <hash_items>      pipeline_machine_assign_items
%type  <hash_item>       pipeline_machine_assign_item

%type  <hash_items>      inner_t_keys
%type  <hash_item>       inner_t_key

%type  <data>            env_t
%type  <data>            hash_t
%type  <data>            named_t
%type  <data>            inner_t
%type  <data>            string_t
%type  <data>            subroutine

%%

start : 
	/* empty */  { *hash = hash_new(1); }
	| statements { *hash = $1; }
	;

/* statements {{{ */
statements :
	  statement            { $$ = hash_new(2); hash_assign_hash_t(&$$[0], &$1); }
	| statements statement { $$ = hash_append($1, $2); };

statement :
	  statement_named_data {
		$$.key  = HK(data);
		$$.data = $1;
	}
	| subroutine {
		$$.key  = HK(machine);
		$$.data = $1;
	}
	;
/* }}} */
/* statement: named data {{{*/
statement_named_data :
	keyword NAME ASSIGN data {
		ssize_t           ret;
		
		if( (ret = data_named_t(&$$, $2, $4)) < 0)
			emit_error("data init failed (ret: %s)", errors_describe(ret));
		
		free($2);
	}
	;

/* }}} */
/* subroutine {{{*/
pipeline :
	pipeline_raw {
		$$.type = TYPE_MACHINET;
		$$.ptr  = pipeline_finalize(&$1);
	}
	;

pipeline_raw :
	pipeline_machine ';' {
		pipeline_new(&$$);
		pipeline_append(&$$, $1);
		hash_free($1);
	}
	| pipeline_raw pipeline_machine ';' {
		$$ = $1;
		pipeline_append(&$$, $2);
		hash_free($2);
	}
	;

/* }}} */
/* subroutine machines {{{*/
pipeline_machine :
	  pipeline_machine_query
	| pipeline_machine_assign
	| pipeline_machine_enum
	;

pipeline_machine_enum :
	data '|' data {
		data_t  d_action;
		data_action_t(&d_action, ACTION_ENUM);
		
		hash_t *r_request = hash_new(3);
		r_request[0].key  = HK(action);
		r_request[0].data = d_action;
		r_request[1].key  = HK(data);
		r_request[1].data = $3;
		
		hash_t r_config[] = {
			{ HK(class),   DATA_PTR_STRING(strdup("data/query"))  },
			{ HK(data),    $1                                     },
			{ HK(request), DATA_PTR_HASHT(r_request)              },
			hash_end
		};
		$$ = hash_rebuild(r_config);
	}
	;

pipeline_machine_query :
	data '.' action_t '(' hash_items ')' {
		data_t  d_action;
		data_action_t(&d_action, $3);
		
		hash_t *r_request = hash_new(3);
		r_request[0].key  = HK(action);
		r_request[0].data = d_action;
		hash_assign_hash_inline(&r_request[1], $5);
		
		hash_t r_config[] = {
			{ HK(class),   DATA_PTR_STRING(strdup("data/query"))  },
			{ HK(data),    $1                                     },
			{ HK(list),    DATA_HEAP_UINTT(1)                     },
			{ HK(request), DATA_PTR_HASHT(r_request)              },
			hash_end
		};
		$$ = hash_rebuild(r_config);
	};

pipeline_machine_assign :
	pipeline_machine_assign_items {
		hash_t r_config[] = {
			{ HK(class),   DATA_PTR_STRING(strdup("assign"))      },
			{ HK(before),  DATA_PTR_HASHT($1)                     },
			hash_end
		};
		$$ = hash_rebuild(r_config);
	}
	;

pipeline_machine_assign_items :
	  pipeline_machine_assign_item                                   { $$ = hash_new(2); hash_assign_hash_t(&$$[0], &$1); }
	| pipeline_machine_assign_items ',' pipeline_machine_assign_item { $$ = hash_append($1, $3); };

pipeline_machine_assign_item :
	env_t_key ASSIGN data { $$.key  = $1; $$.data = $3; };

/* }}} */
/* data {{{ */
data :
	  data_simple
	| data_complex_rules
	;

data_simple :
	  named_t
	| env_t 
	| string_t
	| hash_t
	| inner_t
	| data_converted
	| subroutine
	;

data_complex:
	  data_simple
	| data_complex_rules
	;

/* }}} */
/* data simple: named_t, env_t, string_t hash_t {{{ */
named_t : NAME {
		ssize_t                ret;
		data_t                 d_void            = DATA_VOID;
		
		if( (ret = data_named_t(&$$, $1, d_void)) < 0)
			emit_error("data init failed (ret: %s)", errors_describe(ret));
		
		free($1);
	};
	
env_t : env_t_key {
		ssize_t                ret;
		
		if( (ret = data_env_t(&$$, $1)) < 0)
			emit_error("data init failed (ret: %s)", errors_describe(ret));
	};


string_t : STRING {
		ssize_t                ret;
		data_t                 d_initstr         = DATA_PTR_STRING($1);
		
		data_raw_t_empty(&$$);
		
		fastcall_convert_from r_init_str = { { 5, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(config) }; 
		if( (ret = data_query(&$$, &r_init_str)) < 0)
			emit_error("value init failed from string (ret: %s)", errors_describe(ret));
		
		free($1);
	};

hash_t : '{' hash_items '}' {
		$$.type = TYPE_HASHT;
		$$.ptr = $2;
	};

subroutine :
	SUB '{' pipeline '}' { $$ = $3; };

inner_t :
	data_simple '[' inner_t_keys ']' {
		ssize_t                ret;
		data_t                 d_keys            = DATA_PTR_HASHT($3);
		
		if( (ret = data_inner_t(&$$, $1, d_keys, 1)) < 0)
			emit_error("inner init failed (ret: %s)", errors_describe(ret));
	};

inner_t_keys :
	  inner_t_key              { $$ = hash_new(2); hash_assign_hash_t(&$$[0], &$1); }
	| inner_t_keys inner_t_key { $$ = hash_append($1, $2); };

inner_t_key :
	  hashkey_t {
		ssize_t                ret;
		
		if( (ret = data_hashkey_t(&$$.data, $1)) < 0)
			emit_error("hashkey init failed (ret: %s)", errors_describe(ret));

		$$.key = 0;
	};
/* }}} */
/* data complex and converted {{{ */
data_converted :
	datatype_t ':' data_simple {
		ssize_t                ret;
		
		$$.type = $1;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_convert = {
			{ 5, ACTION_CONVERT_FROM },
			&$3,
			($3.type == TYPE_HASHT) ?
				FORMAT(hash) :
				FORMAT(config)
		}; 
		if( (ret = data_query(&$$, &r_convert)) < 0)
			emit_error("value init failed (ret: %s)", errors_describe(ret));
		
		data_free(&$3);
	};

data_complex_rules :
	NAME '~' data_simple '>' data_complex {
		ssize_t                ret;
		char                  *triplet_type_str      = NULL;
		
		triplet_type_str = strdup("triplet_");
		strcat(triplet_type_str, $1);
		strcat(triplet_type_str, "_t");
		
		datatype_t             type;
		data_t                 d_type            = DATA_PTR_DATATYPET(&type);
		data_t                 d_type_str        = DATA_PTR_STRING(triplet_type_str);
		
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_type_str, FORMAT(config) }; 
		if( (ret = data_query(&d_type, &r_convert)) < 0)
			emit_error("unknown triplet datatype_t \"%s\" (ret: %s)", triplet_type_str, errors_describe(ret));
		
		hash_t                 r_triplet_config[] = {
			{ HK(data),  $5 },
			{ HK(slave), $3 },
			hash_end
		};
		data_t                 d_triplet_config  = DATA_PTR_HASHT(r_triplet_config);
		
		$$.type = type;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_convert_triplet = { { 5, ACTION_CONVERT_FROM }, &d_triplet_config, FORMAT(hash) }; 
		if( (ret = data_query(&$$, &r_convert_triplet)) < 0)
			emit_error("triplet data init failed (ret: %s)", errors_describe(ret));
		
		data_free(&r_triplet_config[0].data);
		data_free(&r_triplet_config[1].data);
		free($1);
	}
	| datatype_t ':' '{' hash_items '}' '>' data_complex {
		ssize_t                ret;
		hash_t                 r_hash[] = {
			{ HK(data), $7 },
			hash_inline($4),
			hash_end
		};
		data_t                 d_hash            = DATA_PTR_HASHT(r_hash);
		
		$$.type = $1;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_hash, FORMAT(hash) }; 
		if( (ret = data_query(&$$, &r_convert)) < 0)
			emit_error("proxy data init failed (ret: %s)", errors_describe(ret));
		
		hash_free($4);
		data_free(&r_hash[0].data);
	}
	;

/* }}} */
/* base atoms: datatype_t, hashkey_t, action_t, env_t {{{ */
action_t : NAME {
		ssize_t                ret;
		data_t                 d_action          = DATA_PTR_ACTIONT(&$$);
		data_t                 d_initstr         = DATA_PTR_STRING($1);
		
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_action, &r_convert)) < 0)
			emit_error("unknown action \"%s\" (ret: %s)", $1, errors_describe(ret));
		
		free($1);
	};

datatype_t : NAME {
		ssize_t                ret;
		data_t                 d_type            = DATA_PTR_DATATYPET(&$$);
		data_t                 d_type_initstr    = DATA_PTR_STRING($1);
		
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_type_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_type, &r_convert)) < 0)
			emit_error("unknown datatype_t \"%s\" (ret: %s)", $1, errors_describe(ret));
		
		free($1);
	}
	;

hashkey_t : NAME {
		ssize_t                ret;
		data_t                 d_key             = DATA_PTR_HASHKEYT(&$$);
		data_t                 d_initstr         = DATA_PTR_STRING($1);
		
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_key, &r_convert)) < 0)
			emit_error("unknown hashkey_t \"%s\" (ret: %s)", $1, errors_describe(ret));
		
		free($1);
	};

env_t_key : '$' hashkey_t { $$ = $2; }

hash_items :
	/* empty */ { $$ = hash_new(1); }
	| hash_item { $$ = hash_new(2); hash_assign_hash_t(&$$[0], &$1); }
	| hash_items ',' hash_item { $$ = hash_append($1, $3); };

hash_item :
	  data {
		$$.key  = 0;
		$$.data = $1;
	}
	| hashkey_t ASSIGN data {
		$$.key  = $1;
		$$.data = $3;
	}
	;
/* }}} */
/* syntax features: keywords {{{ */
keyword : KEYWORD {
		if(config_fz_keyword(&$$, $1) < 0)
			emit_error("unknown keyword \"%s\"", $1);
			
		free($1);
	}
	;
/* }}} */

%%

/*
	| KEYWORD keyword {
		ssize_t                ret;
		
		if(config_fz_keyword(&$$, $1) < 0)
			emit_error("unknown keyword \"%s\"", $1);
			
		free($1);
		
		$$ |= $2;
		if( (ret = config_fz_keyword_validate(&$$)) < 0)
			emit_error("invalid keyword combination (ret: %s)", errors_describe(ret));
	};*/

static void    yyerror(hash_t **hash, const char *msg){ // {{{
	char                  *file              = config_ext_file; 
	
	if(!file)
		file = "-";
	
	fprintf(stderr, "%s: error: %d: %s\n", file, config_get_lineno(), msg);
	(void)hash;
} // }}}
static ssize_t config_fz_keyword(uintmax_t *keyword, char *string){ // {{{
	keypair_t             *kp;
	
	for(kp = fz_keywords; kp->key_str; kp++){
		if(strcasecmp(string, kp->key_str) == 0){
			*keyword = kp->key_val;
			return 0;
		}
	}
	return -EINVAL;
} // }}}
/*static ssize_t config_fz_keyword_validate(uintmax_t *pkeyword){ // {{{
	uintmax_t             keyword           = *pkeyword;
	
	// local/global
	if((keyword & KEYWORD_GLOBAL) != 0){ keyword &= ~KEYWORD_LOCAL; };
	
	*pkeyword = keyword;
	return 0;
} // }}}*/

hash_t *       configs_fz_string_parse(char *string){ // {{{
	hash_t *new_hash = NULL;
	
	#if YYDEBUG
	yydebug = 1;
	#endif

	config__scan_string(string);
	
	yyparse(&new_hash);
	
	config_lex_destroy();
	return new_hash;
} // }}}
