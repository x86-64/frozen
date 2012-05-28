%{
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
static ssize_t       config_fz_keyword_validate(uintmax_t *pkeyword);

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
	hash_t     *entities;
	hash_t      entity;
	
	char       *name;
	data_t      data;
	
	hashkey_t   hashkey;
	datatype_t  datatype;
	uintmax_t   keyword;
}
%token NAME STRING ASSIGN TNULL
%type  <entities>    entities hash_items
%type  <entity>      entity hash_item

%type  <name>        NAME STRING
%type  <data>        data entity_data entity_data_item entity_pipeline

%type  <hashkey>     hashkey
%type  <datatype>    datatype
%type  <keyword>     keyword

%%

start : entities { *hash = $1; }

/* entities {{{ */
entities :
	/* empty */       { $$ = hash_new(1); }
	| entity          { $$ = hash_new(2); hash_assign_hash_t(&$$[0], &$1); }
	| entities entity { $$ = hash_append($1, $2); };

entity :
	  entity_data ';' {
		$$.key  = HK(data);
		$$.data = $1;
	}
	| entity_pipeline ';' {
		$$.key  = HK(machine);
		$$.data = $1;
	}
	;
/* }}} */
/* entity: data {{{*/
entity_data :
	keyword NAME '{' entity_data_item '}' {
		ssize_t           ret;
		
		if( (ret = data_named_t(&$$, $2, $4)) < 0)
			emit_error("data init failed (ret: %s)", errors_describe(ret));
		
		free($2);
	}
	;

entity_data_item:
	/* empty */ {
		data_set_void(&$$);
	}
	| data {
		$$ = $1;
	}
	| datatype '{' hash_items '}' ',' entity_data_item {
		ssize_t                ret;
		hash_t                 r_hash[] = {
			{ HK(data), $6 },
			hash_inline($3),
			hash_end
		};
		data_t                 d_hash            = DATA_PTR_HASHT(r_hash);
		
		$$.type = $1;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_hash, FORMAT(hash) }; 
		if( (ret = data_query(&$$, &r_convert)) < 0)
			emit_error("proxy data init failed (ret: %s)", errors_describe(ret));
		
		hash_free($3);
		data_free(&r_hash[0].data);
	}
	;
/* }}} */
/* entity: pipeline {{{*/
entity_pipeline :
	NAME {
		data_set_void(&$$);
	}
	| entity_pipeline '|' entity_pipeline {
		data_set_void(&$$);
	}
	;
/* }}} */
/* hash {{{ */
hash_items :
	/* empty */ { $$ = hash_new(1); }
	| hash_item { $$ = hash_new(2); hash_assign_hash_t(&$$[0], &$1); }
	| hash_items ',' hash_item { $$ = hash_append($1, $3); };

hash_item :
	  data {
		$$.key  = 0;
		$$.data = $1;
	}
	| hashkey ASSIGN data {
		$$.key  = $1;
		$$.data = $3;
	}
	;

data :
	  STRING             {
		ssize_t                ret;
		data_t                 d_initstr         = DATA_PTR_STRING($1);
		
		data_raw_t_empty(&$$);
		
		fastcall_convert_from r_init_str = { { 5, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(config) }; 
		if( (ret = data_query(&$$, &r_init_str)) < 0)
			emit_error("value init failed from string (ret: %s)", errors_describe(ret));
		
		free($1);
	}
	| '{' hash_items '}' { $$.type = TYPE_HASHT;   $$.ptr = $2; }
	| datatype STRING {
		ssize_t                ret;
		data_t                 d_val_initstr     = DATA_PTR_STRING($2);
		
		$$.type = $1;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_init2 = { { 5, ACTION_CONVERT_FROM }, &d_val_initstr, FORMAT(config) }; 
		if( (ret = data_query(&$$, &r_init2)) < 0)
			emit_error("value init failed \"%s\" (ret: %s)", $2, errors_describe(ret));
		
		free($2);
	}
	| datatype '{' hash_items '}' {
		ssize_t                ret;
		data_t                 d_hash            = DATA_PTR_HASHT($3);
		
		$$.type = $1;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_hash, FORMAT(hash) }; 
		if( (ret = data_query(&$$, &r_convert)) < 0)
			emit_error("value init failed (ret: %s)", errors_describe(ret));
		
		hash_free($3);
	};
/* }}} */
/* base types: datatype, hashkey, keywords {{{ */
datatype : NAME {
		ssize_t                ret;
		data_t                 d_type            = DATA_PTR_DATATYPET(&$$);
		data_t                 d_type_initstr    = DATA_PTR_STRING($1);
		
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_type_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_type, &r_convert)) < 0)
			emit_error("unknown datatype_t \"%s\" (ret: %s)", $1, errors_describe(ret));
		
		free($1);
	}
	| '(' datatype ')' {
		$$ = $2;
	};

hashkey : NAME {
		ssize_t                ret;
		data_t                 d_key             = DATA_PTR_HASHKEYT(&$$);
		data_t                 d_initstr         = DATA_PTR_STRING($1);
		
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_key, &r_convert)) < 0)
			emit_error("unknown hashkey_t \"%s\" (ret: %s)", $1, errors_describe(ret));
		
		free($1);
	};

keyword : NAME {
		if(config_fz_keyword(&$$, $1) < 0)
			emit_error("unknown keyword \"%s\"", $1);
			
		free($1);
	}
	| keyword keyword {
		ssize_t                ret;
		
		$$ = $1 | $2;
		if( (ret = config_fz_keyword_validate(&$$)) < 0)
			emit_error("invalid keyword combination (ret: %s)", errors_describe(ret));
	};
/* }}} */

%%

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
static ssize_t config_fz_keyword_validate(uintmax_t *pkeyword){ // {{{
	uintmax_t             keyword           = *pkeyword;
	
	// local/global
	if((keyword & KEYWORD_GLOBAL) != 0){ keyword &= ~KEYWORD_LOCAL; };
	
	*pkeyword = keyword;
	return 0;
} // }}}

hash_t *       configs_fz_string_parse(char *string){ // {{{
	hash_t *new_hash = NULL;
	
	config__scan_string(string);
	
	yyparse(&new_hash);
	
	config_lex_destroy();
	return new_hash;
} // }}}
