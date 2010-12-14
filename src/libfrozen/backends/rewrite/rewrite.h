#define MAX_ALLOCA_SIZE 0x1000

typedef enum rewrite_actions {
	VALUE_SET,
	VALUE_LENGTH,
	CALL_BACKEND,
	CALL_PASS,
	DATA_LENGTH,
	DATA_ARITH,
	DATA_CONVERT,
	DATA_ALLOCA,
	DATA_FREE,
	
#ifdef DEBUG
	HASH_DUMP,
#endif

	DO_NOTHING = -1
} rewrite_actions;

typedef enum things {
	THING_ARRAY_REQUEST,
	THING_ARRAY_REQUEST_KEY,
	THING_CONST,
	THING_VARIABLE,
	THING_LIST,
	THING_RET
} things;

typedef struct rewrite_thing_t rewrite_thing_t;
struct rewrite_thing_t {
	things            type;
	rewrite_thing_t  *next;
	
	// THING_ARRAY_REQUEST
	char             *array_key;
	
	// THING_CONST or THING_VARIABLE
	unsigned int      id;
	
	// THING_LIST
	rewrite_thing_t  *list;
};

typedef struct rewrite_variable_t {
	unsigned int          id;
	data_t                data;
	data_ctx_t           *data_ctx;
} rewrite_variable_t;

typedef struct rewrite_action_t {
	unsigned int          id;
	rewrite_actions       action;
	rewrite_thing_t      *params;
	rewrite_thing_t      *ret;
} rewrite_action_t;

typedef struct rewrite_script_t {
	char                 *script;
	
	rewrite_action_t     *actions;
	unsigned int          actions_count;
	rewrite_variable_t   *constants;
	unsigned int          constants_count;
	unsigned int          variables_count;
	unsigned int          requests_count;
	
} rewrite_script_t;

typedef struct rewrite_script_env_t {
	rewrite_script_t     *script;
	rewrite_variable_t   *variables;
	request_t           **requests;
	data_t               *ret_data;
} rewrite_script_env_t;

ssize_t  rewrite_script_parse (rewrite_script_t *script, char *string);
void     rewrite_script_free  (rewrite_script_t *script);

