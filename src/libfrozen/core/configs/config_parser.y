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

char  defconfig_m4_path[] = M4PATH;
char  defconfig_m4_incs[] = "";
char  defconfig_m4_opts[] = "";

char *config_m4_path      = defconfig_m4_path;
char *config_m4_incs      = defconfig_m4_incs;
char *config_m4_opts      = defconfig_m4_opts;

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
		hash_assign_hash_t   (&$1[nelements-1], &$3);
		hash_assign_hash_end (&$1[nelements  ]);
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
		ssize_t                ret;
		data_t                 d_key             = DATA_PTR_HASHKEYT(&$$);
		data_t                 d_initstr         = DATA_PTR_STRING($1);

		fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_key, &r_convert)) < 0)
			emit_error("unknown hashkey_t \"%s\" (ret: %s)", $1, errors_describe(ret));
		
		free($1);
	};

hash_value :
	  STRING             {
		ssize_t                ret;
		data_t                 d_initstr    = DATA_PTR_STRING($1);
		
		$$.type = TYPE_RAWT;
		$$.ptr  = NULL;
		
		fastcall_convert_from r_init_str = { { 4, ACTION_CONVERT_FROM }, &d_initstr, FORMAT(config) }; 
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
		
		fastcall_convert_from r_init1 = { { 4, ACTION_CONVERT_FROM }, &d_type_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_type, &r_init1)) < 0)
			emit_error("unknown datatype_t \"%s\" (ret: %s)", $2, errors_describe(ret));
		
		$$.type = type;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_init2 = { { 4, ACTION_CONVERT_FROM }, &d_val_initstr, FORMAT(config) }; 
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

		fastcall_convert_from r_init1 = { { 4, ACTION_CONVERT_FROM }, &d_type_initstr, FORMAT(config) }; 
		if( (ret = data_query(&d_type, &r_init1)) < 0)
			emit_error("unknown datatype_t \"%s\" (ret: %s)", $2, errors_describe(ret));
		
		$$.type = type;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, &d_hash, FORMAT(hash) }; 
		if( (ret = data_query(&$$, &r_convert)) < 0)
			emit_error("data init failed \"%s\" (ret: %s)", $2, errors_describe(ret));
		
		free($2);
		hash_free($5);
	};

%%

void yyerror(hash_t **hash, const char *msg){ // {{{
	char                  *file              = config_ext_file; 
	
	if(!file)
		file = "-";
	
	fprintf(stderr, "%s: error: %d: %s\n", file, config_get_lineno(), msg);
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
	uintmax_t              is_process        = 0;

	ext = strrchr(filename, '.');
	if(ext != NULL && strcmp(ext, ".m4") == 0){
		char buffer[DEF_BUFFER_SIZE];
		
		if(snprintf(
			buffer, sizeof(buffer),
			"%s %s -s %s %s",
			config_m4_path,
			config_m4_incs,
			config_m4_opts,
			filename
		) > sizeof(buffer)) // -s for sync lines
			return NULL;
		
		if( (fd = popen(buffer, "r")) == NULL)
			return NULL;
		
		is_process = 1;
	}else{
		if( (fd = fopen(filename, "rb")) == NULL)
			return NULL;
		
	}
	
	while(!feof(fd)){
		content       = realloc(content, content_off + DEF_BUFFER_SIZE + 1); // 1 for terminating \0
		if(!content)
			break;
		
		content_off  += fread(content + content_off, 1, DEF_BUFFER_SIZE, fd);
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

