%{
#include <libfrozen.h>
#include <configs/config.h>
#include <configs/config_parser.tab.h>	

void yyerror (hash_t **, const char *);

typedef struct yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE config__scan_string (const char *string);  
extern int config_lex_destroy(void);
extern int config_lex(YYSTYPE *);

%}

%start  start

%define api.pure
%parse-param {hash_t **hash}

%union {
	hash_t     *hash_items;
	hash_t      hash_item;
	hashkey_t  key;
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
		hashkey_t   key;
		data_t       d_key    = DATA_PTR_HASHKEYT(&key);
	
		fastcall_init r_init1 = { { 3, ACTION_INIT }, $1 }; 
		if(data_query(&d_key, &r_init1) != 0){
			yyerror(hash, "failed convert hashkey\n"); YYERROR;
		}

		if( ($$ = key) == 0){
			printf("unknown key: %s\n", $1); YYERROR;
		}
		free($1);
	};

hash_value :
	  STRING             { $$.type = TYPE_STRINGT; $$.ptr = $1; }
	| '{' hash_items '}' { $$.type = TYPE_HASHT;   $$.ptr = $2; }
	| '(' NAME ')' STRING {
		datatype_t  type;
		data_t      d_type   = DATA_PTR_DATATYPET(&type);
		
		fastcall_init r_init1 = { { 3, ACTION_INIT }, $2 }; 
		if(data_query(&d_type, &r_init1) != 0){
			yyerror(hash, "failed convert datatype\n"); YYERROR;
		}
		
		$$.type = type;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_init r_init2 = { { 3, ACTION_INIT }, $4 }; 
		if(data_query(&$$, &r_init2) != 0){
			char buffer[DEF_BUFFER_SIZE];
			
			snprintf(buffer, sizeof(buffer), "failed convert data: (%s)'%s'\n", $2, $4);

			yyerror(hash, buffer); YYERROR;
		}
		
		free($2);
		free($4);
	}
	| '(' NAME ')' '{' hash_items '}' {
		datatype_t  type;
		data_t      d_type   = DATA_PTR_DATATYPET(&type);
		data_t      d_hash   = DATA_PTR_HASHT($5);
		
		fastcall_init r_init1 = { { 3, ACTION_INIT }, $2 }; 
		if(data_query(&d_type, &r_init1) != 0){
			yyerror(hash, "failed convert datatype\n"); YYERROR;
		}
		
		$$.type = type;
		$$.ptr  = NULL;
		
		/* convert string to needed data */
		fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, &d_hash, FORMAT(hash) }; 
		if(data_query(&$$, &r_convert) != 0){
			char buffer[DEF_BUFFER_SIZE];
			
			snprintf(buffer, sizeof(buffer), "failed convert data: (%s)\n", $2);

			yyerror(hash, buffer); YYERROR;
		}
		
		free($2);
		hash_free($5);

	}
	| NAME {
		data_functions action;
		if((action = request_str_to_action($1)) != ACTION_INVALID){
			data_t d_act = DATA_UINT32T(action);
			
			fastcall_copy r_copy = { { 3, ACTION_COPY }, &$$ };
			data_query(&d_act, &r_copy);

			free($1);
		}else{
			yyerror(hash, "wrong constant\n"); YYERROR;
		}
     };

%%

void yyerror(hash_t **hash, const char *msg){
	(void)hash;
	fprintf(stderr, "config error: %s\n", msg);
}

hash_t *   configs_string_parse(char *string){ // {{{
	hash_t *new_hash = NULL;
	
	config__scan_string(string);
	
	yyparse(&new_hash);
	
	config_lex_destroy();
	return new_hash;
} // }}}

hash_t *   configs_file_parse(char *filename){ // {{{
	int     size = 0;
	hash_t *new_hash = NULL;
	char   *string;
	FILE   *f;
	
	if( (f = fopen(filename, "rb")) == NULL)
		return NULL;
	
	fseek(f, 0, SEEK_END); size = ftell(f); fseek(f, 0, SEEK_SET);
	
	if( (string = malloc(size+1)) != NULL){
		if(fread(string, sizeof(char), size, f) == size){
			string[size] = '\0';
			new_hash = configs_string_parse(string);
		} 
		free(string);
	}
	fclose(f);
	return new_hash;
} // }}}

