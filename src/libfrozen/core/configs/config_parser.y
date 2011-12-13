%{
#include <libfrozen.h>
#include <configs/config.h>
#include <configs/config_parser.tab.h>	

void yyerror (hash_t **, const char *);

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE config__scan_string (const char *string);  
extern int config_lex_destroy(void);
extern int config_lex(YYSTYPE *);
extern int config_get_lineno(void);
extern char *config_get_text(void);
extern char *config_ext_file;

#define emit_error(fmt, ...){                                           \
	do {                                                            \
		char _buffer[DEF_BUFFER_SIZE];                          \
		                                                        \
		snprintf(_buffer, sizeof(_buffer), fmt, ##__VA_ARGS__); \
		yyerror(hash, _buffer);                                 \
		YYERROR;                                                \
	}while(0); }

%}

%start  start

%define api.pure
%parse-param {hash_t **hash}

%union {
	hash_t     *hash_items;
	hash_t      hash_item;
	hashkey_t   key;
	char       *name;
	data_t      data;
}
%token NAME STRING ASSIGN TNULL
%type  <hash_items>  hash_items
%type  <hash_item>   hash_item
%type  <key>         hash_name
%type  <name>        NAME STRING
%type  <data>        hash_value

%%

start : hash_items { *hash = $1; }

hash_items :
	/* empty */ {
		$$ = malloc(sizeof(hash_t));
		hash_assign_hash_end($$);
	}
	| hash_item {
		$$ = malloc(2 * sizeof(hash_t));
		hash_assign_hash_t   (&$$[0], &$1);
		hash_assign_hash_end (&$$[1]);
	}
	| hash_items ',' hash_item {
		size_t nelements = hash_nelements($1);
		$1 = realloc($1, (nelements + 1) * sizeof(hash_t));
		memmove(&$1[1], &$1[0], nelements * sizeof(hash_t));
		hash_assign_hash_t(&$1[0], &$3);
		$$ = $1;
	}
	;

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
		data_t                 d_key             = DATA_PTR_HASHKEYT(&$$);
		
		fastcall_init r_init1 = { { 3, ACTION_INIT }, $1 }; 
		if(data_query(&d_key, &r_init1) != 0)
			emit_error("unknown hashkey_t (%s)", $1);
		
		free($1);
	};

hash_value :
	  STRING             { $$.type = TYPE_STRINGT; $$.ptr = $1; }
	| '{' hash_items '}' { $$.type = TYPE_HASHT;   $$.ptr = $2; }
	| '(' NAME ')' STRING {
		data_t                 d_type            = DATA_PTR_DATATYPET(&$$.type);
		
		fastcall_init r_init1 = { { 3, ACTION_INIT }, $2 }; 
		if(data_query(&d_type, &r_init1) != 0)
			emit_error("unknown datatype_t (%s)", $2);
		
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_init r_init2 = { { 3, ACTION_INIT }, $4 }; 
		if(data_query(&$$, &r_init2) != 0)
			emit_error("data init failed (%s)", $2);
		
		free($2);
		free($4);
	}
	| '(' NAME ')' '{' hash_items '}' {
		data_t                 d_type            = DATA_PTR_DATATYPET(&$$.type);
		data_t                 d_hash            = DATA_PTR_HASHT($5);
		
		fastcall_init r_init1 = { { 3, ACTION_INIT }, $2 }; 
		if(data_query(&d_type, &r_init1) != 0)
			emit_error("unknown datatype_t (%s)", $2);
		
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, &d_hash, FORMAT(hash) }; 
		if(data_query(&$$, &r_convert) != 0)
			emit_error("data init failed (%s)", $2);
		
		free($2);
		hash_free($5);
	}
	| NAME {
			// TODO remove this
		data_functions action;
		if((action = request_str_to_action($1)) != ACTION_INVALID){
			data_t d_act = DATA_UINT32T(action);
			
			fastcall_copy r_copy = { { 3, ACTION_COPY }, &$$ };
			data_query(&d_act, &r_copy);
			
			free($1);
		}else{
			emit_error("wrong constant");
		}
     };

%%

void yyerror(hash_t **hash, const char *msg){ // {{{
	char                  *file              = config_ext_file; 
	
	if(!file)
		file = "-";
	
	fprintf(stderr, "%s: error: %d: %s near '%s'\n", file, config_get_lineno(), msg, config_get_text());
	(void)hash;
} // }}}

hash_t *   configs_string_parse(char *string){ // {{{
	hash_t *new_hash = NULL;
	
	config__scan_string(string);
	
	yyparse(&new_hash);
	
	config_lex_destroy();
	return new_hash;
} // }}}
hash_t *   configs_file_parse(char *filename){ // {{{
	hash_t                *new_hash          = NULL;
	FILE                  *fd;
	char                  *ext;
	char                  *content           = NULL;
	uintmax_t              content_off       = 0;
	uintmax_t              content_size      = 0;
	uintmax_t              is_process        = 0;

	ext = strrchr(filename, '.');
	if(ext != NULL && strcmp(ext, ".m4") == 0){
		char buffer[DEF_BUFFER_SIZE];
		
		if(snprintf(buffer, sizeof(buffer), "%s -s %s", M4PATH, filename) > sizeof(buffer)) // -s for sync lines
			return NULL;
		
		if( (fd = popen(buffer, "r")) == NULL)
			return NULL;
		
		is_process = 1;
	}else{
		if( (fd = fopen(filename, "rb")) == NULL)
			return NULL;
		
	}
	
	while(!feof(fd)){
		content_size += DEF_BUFFER_SIZE;
		content       = realloc(content, content_size + 1); // 1 for terminating \0
		if(!content)
			break;
		
		content_off  += fread(content + content_off, 1, content_size - content_off, fd);
	}
	if(is_process == 1) pclose(fd); else fclose(fd);
	
	if(content){
		config_ext_file = strdup(filename);
		
		content[content_off] = '\0';
		
		new_hash = configs_string_parse(content);
		free(content);
		
		if(config_ext_file){
			free(config_ext_file);
			config_ext_file = NULL;
		}
	}
	return new_hash;
} // }}}

