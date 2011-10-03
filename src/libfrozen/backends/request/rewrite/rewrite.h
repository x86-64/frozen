#define MAX_ALLOCA_SIZE 0x1000

typedef enum rewrite_actions {
	LANG_SET,
	LANG_IF,
	BACKEND_QUERY,
	BACKEND_PASS,
	DATA_QUERY,

#ifdef DEBUG
	HASH_DUMP,
#endif

	DO_NOTHING = -1
} rewrite_actions;

typedef enum things {
	THING_ACTION_BLOCK,
	THING_HASHT,
	THING_HASH_ELEMENT,
	THING_CONST,
	THING_VARIABLE,
	THING_LIST,
	THING_RET,
} things;

typedef struct rewrite_action_t       rewrite_action_t;
typedef struct rewrite_action_block_t rewrite_action_block_t;
typedef struct rewrite_name_t         rewrite_name_t;
typedef struct rewrite_thing_t        rewrite_thing_t;
typedef struct rewrite_variable_t     rewrite_variable_t;

struct rewrite_thing_t {
	things                   type;
	rewrite_thing_t         *next;
	
	union {
		struct {
			// THING_HASHT_KEY
			hash_key_t               array_key;

			// THING_HASHT or THING_CONST or THING_VARIABLE
			unsigned int             id;
		};
		// THING_ACTION_BLOCK
		rewrite_action_block_t  *block;

		// THING_LIST
		rewrite_thing_t         *list;
	};
};

struct rewrite_variable_t {
	unsigned int             id;
	data_t                   data;
};

struct rewrite_name_t {
	rewrite_name_t          *next;
	char                    *name;
	things                   type;
	unsigned int             id;
};

struct rewrite_action_t {
	unsigned int             id;
	rewrite_actions          action;
	rewrite_thing_t         *params;
	rewrite_thing_t         *ret;
};

struct rewrite_action_block_t {
	rewrite_action_t        *actions;
	unsigned int             actions_count;
	
	rewrite_action_block_t  *next;
};

typedef struct rewrite_script_t {
	char                    *script;
	
	rewrite_action_block_t  *main;
	rewrite_variable_t      *constants;
	unsigned int             constants_count;
	unsigned int             variables_count;
	unsigned int             requests_count;
	
	rewrite_name_t          *names;
} rewrite_script_t;

typedef struct rewrite_script_env_t {
	rewrite_script_t        *script;
	rewrite_variable_t      *variables;
	request_t              **requests;
	data_t                  *ret_data;
	backend_t                 *backend;

} rewrite_script_env_t;

ssize_t  rewrite_script_parse (rewrite_script_t *script, char *string);
void     rewrite_script_free  (rewrite_script_t *script);

