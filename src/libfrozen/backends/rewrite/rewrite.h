#define MAX_ALLOCA_SIZE 0x1000

typedef enum rewrite_actions {
	VALUE_SET,
	VALUE_UNSET,
	VALUE_LENGTH,
	CALL_BACKEND,
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
	THING_NOTHING,
	THING_REQUEST,
	THING_CONFIG
} things;

typedef enum rewrite_times {
	TIME_BEFORE,
	TIME_AFTER
} rewrite_times;

typedef struct target_t {
	things        type;
	
	// optional
	char         *subkey;
	unsigned int  rule_num;
	data_t        data;
	data_ctx_t   *data_ctx;
} target_t;

typedef struct rewrite_rule_t {
	request_actions       filter;
	rewrite_times         time;
	rewrite_actions       action;
	
	// source
		target_t      src;
		unsigned int  src_info;
		
	// destination
		target_t      dst;
		
		data_type     dst_key_type;
		size_t        dst_key_size;
		unsigned int  dst_info;
		
	// rest
		char          operator;
		
		backend_t    *backend;
		request_t    *request_proto;
		int           ret_override;
		
		unsigned int  copy;
		unsigned int  fatal;
} rewrite_rule_t;

int  rewrite_rule_parse (rewrite_rule_t *new_rule, char *string);
void rewrite_rule_free  (rewrite_rule_t *rule);

