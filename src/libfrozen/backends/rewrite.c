/**
 * @file rewrite.c
 * @ingroup modules
 * @brief Rewrite module
 *
 * Rewrite module can modify requests and buffers with given set of rules
 */

/**
 * @ingroup modules
 * @addtogroup mod_rewrite Module 'rewrite'
 */

/**
 * @ingroup mod_rewrite
 * @page page_rewrite_config Rewrite configuration
 * 
 * Accepted configuration:
 * @code
 * 	settings = {
 * 		rules = {               // required rules list
 *                   <rewrite rules>
 * 		}
 * 	}
 * @endcode
 */

/**
 * @ingroup mod_rewrite
 * @page page_rewrite_rules Rewrite rules
 * 
 * Rewrite rule can contain action, source and destination targets and optional parameters.
 * Source and destination targets defined as follows:
 * @code
 * 	{ src_key     = "source_key"                              }  // for request key
 * 	{ src_config  = "config_key"                              }  // for config key
 * 	{ src_backend = "backend_key", src_rule = 2               }  // for 2nd rule backend's request_proto key
 * 	{ src_key     = "source_key",  src_info = 1,              }  // for data in request key
 * 	{ src_config  = "source_key",  src_info = 1,              }  // for data in config key
 * 	{ src_backend = "source_key",  src_info = 1, src_rule = 2 }  // for data in backend's request_proto key
 *
 * 	{ dst_key     = "destination_key"                                 }  // for request key
 * 	{ dst_key     = "target_key",  dst_type = "binary", dst_size = 10 }  // for dst key with unknown source type and size
 * 	{ dst_backend = "backend_key", dst_rule = 2                       }  // for 2nd rule backend's request_proto key
 * 	{ dst_key     = "destination_key", dst_info = 1,                  }  // for data in request key
 * 	{ dst_backend = "destination_key", dst_info = 1, dst_rule = 2     }  // for data in backend's request_proto key
 * @endcode
 *  
 * Action can be one of following:
 * 	- "set"           - read from src and write it to dst
 * 		@param[in]  src_*    Source target
 * 		@param[out] dst_*    Destination target
 *
 * 	- "unset"         - unset key
 * 		@param[out] dst_*    Destination target
 *
 * 	- "length"        - get length of source (key->value_size). Output type is TYPE_SIZET
 * 		@param[in]  src_*    Source target
 * 		@param[out] dst_*    Destination target
 *
 * 	- "backend"       - call backend
 * 		@param[in]  backend         Backend name or config
 * 		@param[in]  request_proto   Prototype of request
 *
 * 	- "data_length"   - calc length of data_t. Output type is TYPE_SIZET
 * 		@param[in]  src_*    Source target
 * 		@param[out] dst_*    Destination target
 *
 * 	- "data_arith"    - do arithmetic with targets. [ Dst = Dst (operation) Src ]
 * 		@param      dst_*      Destination target
 * 		@param[in]  operation  Operation to do: '+' '-' '*' '/' [TYPE_STRING]
 * 		@param[in]  src_*      Source target
 *
 * 	- "data_convert"  - convert targets
 * 		@param[out] dst_*      Destination target
 * 		@param[in]  src_*      Source target
 *
 * Other parameters:
 * 	@param[in] on-request  Do action only on this ACTION_* supplied [TYPE_INT32]
 * 	@param[in] before      Do request before calling next chain (default)
 * 	@param[in] after       Do request after  calling next chain
 * 	@param[in] fatal       Return error on failed rule
 * 	@param[in] copy        Do action on copy of source target (useful for arithmetic)
 */

#include <libfrozen.h>
#include <alloca.h>

/* typedefs {{{ */
typedef enum rewrite_actions {
	VALUE_SET,
	VALUE_UNSET,
	VALUE_LENGTH,
	CALL_BACKEND,
	DATA_LENGTH,
	DATA_ARITH,
	DATA_CONVERT,
	DATA_FREE,

#ifdef DEBUG
	HASH_DUMP,
#endif

	DO_NOTHING = -1
} rewrite_actions;

typedef enum things {
	THING_NOTHING,
	
	THING_KEY,
	THING_CONFIG,
	THING_REQUEST
} things;

typedef enum rewrite_times {
	TIME_BEFORE,
	TIME_AFTER
} rewrite_times;

typedef struct rewrite_rule_t {
	request_actions       filter;
	rewrite_times         time;
	rewrite_actions       action;
	
	// source
		things        src_type;
		
		char         *src_key;
		data_t        src_config;
		data_ctx_t   *src_config_ctx;
		
		unsigned int  src_info;
		unsigned int  src_rule_num;
		
	// destination
		things        dst_type;
		
		char         *dst_key;
		data_type     dst_key_type;
		size_t        dst_key_size;
		
		unsigned int  dst_info;
		unsigned int  dst_rule_num;
		
	// rest
		char          operator;
		
		backend_t    *backend;
		request_t    *request_proto;
		
		unsigned int  copy;
		unsigned int  fatal;
} rewrite_rule_t;

typedef struct rewrite_user_data {
	rewrite_rule_t   *rules;
	unsigned int      rules_count;
} rewrite_user_data;
/* }}} */

static rewrite_actions  rewrite_str_to_action(char *string){ // {{{
	if(strcasecmp(string, "set")            == 0) return VALUE_SET;
	if(strcasecmp(string, "unset")          == 0) return VALUE_UNSET;
	if(strcasecmp(string, "length")         == 0) return VALUE_LENGTH;
	if(strcasecmp(string, "backend")        == 0) return CALL_BACKEND;
	if(strcasecmp(string, "data_length")    == 0) return DATA_LENGTH;
	if(strcasecmp(string, "data_arith")     == 0) return DATA_ARITH;
	if(strcasecmp(string, "data_convert")   == 0) return DATA_CONVERT;
	if(strcasecmp(string, "data_free")      == 0) return DATA_FREE;

#ifdef DEBUG
	if(strcasecmp(string, "hash_dump")      == 0) return HASH_DUMP;
#endif
	return DO_NOTHING;
} // }}}

static void rewrite_rule_free(rewrite_rule_t *rule){ // {{{
	if(rule->src_key != NULL) free(rule->src_key);
	if(rule->dst_key != NULL) free(rule->dst_key);
	
	data_free(&rule->src_config);
	
	if(rule->request_proto != NULL)
		free(rule->request_proto);
	if(rule->backend != NULL)
		backend_destroy(rule->backend);
	
} // }}}
static int  rewrite_rule_parse(hash_t *rules, void *p_data, void *null2){ // {{{
	void                *temp;
	hash_t              *htemp, *config;
	rewrite_rule_t       new_rule = {0};
	rewrite_user_data   *data      = (rewrite_user_data *)p_data; 
	
	if(hash_get_data_type(rules) != TYPE_HASHT)
		return ITER_BREAK;
	
	config = hash_get_value_ptr(rules);
	
	new_rule.src_type      = THING_NOTHING;
	new_rule.dst_type      = THING_NOTHING;
	
	// parse action {{{
	if(hash_get_typed(config, TYPE_STRING, "action", &temp, NULL) != 0)
		label_error(cleanup, "rewrite rule: 'action' not supplied\n");
	new_rule.action = rewrite_str_to_action(temp);
	
	// }}}
	// parse filter {{{
	new_rule.filter =
		( (htemp = hash_find_typed(config, TYPE_INT32, "on-request")) != NULL) ?
		HVALUE(htemp, unsigned int) : REQUEST_INVALID;
	// }}}
	// parse after/before {{{
	new_rule.time = TIME_BEFORE;
	if( hash_find(config, "before") != NULL)
		new_rule.time = TIME_BEFORE;
	if( hash_find(config, "after")  != NULL)
		new_rule.time = TIME_AFTER;
	// }}}
	// parse fatal {{{
	new_rule.fatal = 0;
	if( hash_find(config, "fatal") != NULL)
		new_rule.fatal = 1;
	// }}}
	// parse copy {{{
	new_rule.copy = 0;
	if( hash_find(config, "copy") != NULL)
		new_rule.copy = 1;
	// }}}
	// parse operator and operand {{{
	new_rule.operator  = 0;
	if( hash_get_typed(config, TYPE_STRING, "operator", &temp, NULL) == 0){
		new_rule.operator  = *(char *)temp;
	}
	// }}}
	// parse request_proto {{{
	hash_t     request_null[] = { hash_end };
	request_t *request_proto;
	size_t     request_proto_size;
	
	if( (htemp = hash_find_typed(config, TYPE_HASHT, "request_proto")) != NULL){
		request_proto      = hash_get_value_ptr(htemp);
		request_proto_size = hash_get_value_size(htemp);
	}else{
		request_proto      = request_null;
		request_proto_size = sizeof(request_null);
	}
	new_rule.request_proto = malloc(request_proto_size);
	memcpy(new_rule.request_proto, request_proto, request_proto_size);
	// }}}
	// parse backend {{{
	if( (htemp = hash_find(config, "backend")) != NULL){
		if( (new_rule.backend = backend_new(htemp)) == NULL)
			return_error(-EINVAL, "rewrite rule: parameter 'backend' invalid\n");
	}
	// }}}
	// src parse {{{
	if( hash_get_typed(config, TYPE_STRING, "src_key", &temp, NULL) == 0){
		new_rule.src_type     = THING_KEY;
		new_rule.src_key      = strdup(temp);
	}
	if( hash_get_typed(config, TYPE_STRING, "src_backend", &temp, NULL) == 0){
		new_rule.src_type     = THING_REQUEST;
		new_rule.src_key      = strdup(temp);
		if( (htemp = hash_find_typed(config, TYPE_INT32, "src_rule")) == NULL)
			label_error(cleanup, "rewrite rule: 'src_rule' not supplied for 'src_backend'\n");
		
		new_rule.src_rule_num = HVALUE(htemp, unsigned int) - 1;
	}
	if( hash_get_typed(config, TYPE_STRING, "src_config", &temp, NULL) == 0){
		data_t *my_config;

		new_rule.src_type     = THING_CONFIG;
		if( (my_config = hash_get_data(config, (char *)temp)) == NULL)
			label_error(cleanup, "rewrite rule: 'src_config' have wrong parameter\n");
		
		if(data_copy(&new_rule.src_config, my_config) != 0)
			label_error(cleanup, "rewrite rule: 'src_config' too big\n");
		
		new_rule.src_config_ctx = hash_get_data_ctx(config, (char *)temp);
	}
	new_rule.src_info =
		( (htemp = hash_find(config, "src_info")) != NULL) ?
		1 : 0;
	// src parse }}}
	// dst parse {{{
	if( hash_get_typed(config, TYPE_STRING, "dst_key", &temp, NULL) == 0){
		new_rule.dst_type     = THING_KEY;
		new_rule.dst_key      = strdup(temp);
	}
	if( hash_get_typed(config, TYPE_STRING, "dst_backend", &temp, NULL) == 0){
		new_rule.dst_type     = THING_REQUEST;
		new_rule.dst_key      = strdup(temp);
		
		if( (htemp = hash_find_typed(config, TYPE_INT32, "dst_rule")) == NULL)
			label_error(cleanup, "rewrite rule: 'dst_rule' not supplied for 'dst_backend'\n");
		
		new_rule.dst_rule_num = HVALUE(htemp, unsigned int) - 1;
	}
	new_rule.dst_info =
		( (htemp = hash_find(config, "dst_info")) != NULL) ?
		1 : 0;
	new_rule.dst_key_type =
		( (htemp = hash_find_typed(config, TYPE_STRING, "dst_type")) != NULL) ?
		data_type_from_string(hash_get_value_ptr(htemp)) : TYPE_INVALID;
	new_rule.dst_key_size =
		( (htemp = hash_find_typed(config, TYPE_SIZET, "dst_size")) != NULL) ?
		HVALUE(htemp, size_t) : (size_t)-1;
	// dst parse }}}
	
	// check parameters {{{
	switch(new_rule.action){
		case DATA_ARITH:
			if(new_rule.operator == 0)
				label_error(cleanup, "rewrite rule: missing operator\n");
			
			/* fallthrough */
		case VALUE_SET:
		case VALUE_LENGTH:
		case DATA_LENGTH:
		case DATA_CONVERT:
			if(new_rule.src_type == THING_NOTHING)
				label_error(cleanup, "rewrite rule: missing src target\n");
			
			/* fallthrough */
		case VALUE_UNSET:
			if(new_rule.dst_type == THING_NOTHING)
				label_error(cleanup, "rewrite rule: missing dst target\n");
			break;
		case CALL_BACKEND:
			if(new_rule.backend == NULL || new_rule.request_proto == NULL)
				label_error(cleanup, "rewrite rule: missing backend or request_proto\n");
			break;
		case DO_NOTHING:
			break;
		case DATA_FREE:
			if(new_rule.src_type == THING_NOTHING)
				label_error(cleanup, "rewrite rule: missing src target\n");
			break;
		
	#ifdef DEBUG
		case HASH_DUMP:
			break;
	#endif
	};
	/*
		if(new_rule.dst_type == THING_KEY){
			if(
				new_rule.src_type == THING_BUFFER &&
				( new_rule.dst_key_type == TYPE_INVALID || new_rule.dst_key_size == 0)
			)
				label_error(cleanup, "rewrite rule: can't fill dst key. supply dst_type and dst_size\n");
		}
	*/
	// }}}
	
	// copy rule
	unsigned int rule_id = data->rules_count++;
	data->rules = realloc(data->rules, data->rules_count * sizeof(rewrite_rule_t));
	memcpy(&data->rules[rule_id], &new_rule, sizeof(new_rule));
	
	return ITER_CONTINUE;
cleanup:
	rewrite_rule_free(&new_rule);
	return ITER_BREAK;
} // }}}


/* init {{{ */
static int rewrite_init(chain_t *chain){ // {{{
	if( (chain->user_data = calloc(1, sizeof(rewrite_user_data))) == NULL)
		return -ENOMEM;
	return 0;
} // }}}
static int rewrite_destroy(chain_t *chain){ // {{{
	unsigned int       i;
	rewrite_user_data *data = (rewrite_user_data *)chain->user_data;
	
	for(i=0; i<data->rules_count; i++){
		rewrite_rule_free(&data->rules[i]);
	}
	if(data->rules != NULL)      // TODO iter and free
		free(data->rules);
	
	free(data);
	chain->user_data = NULL;
	return 0;
} // }}}
static int rewrite_configure(chain_t *chain, hash_t *config){ // {{{
	hash_t            *r_rules, *v_rules;
	rewrite_user_data *data = (rewrite_user_data *)chain->user_data;
	
	if( (r_rules = hash_find_typed(config, TYPE_HASHT, "rules")) == NULL)
		return_error(-EINVAL, "chain 'rewrite' parameter 'rules' not supplied\n");
	
	v_rules = hash_get_value_ptr(r_rules);
	
	if( hash_iter(v_rules, &rewrite_rule_parse, (void *)data, NULL) == ITER_BREAK)
		return -EINVAL;
	
	return 0;
} // }}}
/* }}} */

static request_t ** find_proto(rewrite_user_data *data, request_t **protos, unsigned int rule_num){
	if(rule_num >= data->rules_count)
		return NULL;
	
	return &protos[rule_num];
}

static ssize_t rewrite_func_one(chain_t *chain, request_t *request, rewrite_times pass, request_t **protos){
	unsigned int        i;
	int                 have_after;
	ssize_t             ret;
	//size_t              dst_size;
	//data_type           dst_type;
	hash_t             *r_action;
	request_t          *new_request;
	rewrite_rule_t     *rule;
	rewrite_user_data  *data      = (rewrite_user_data *)chain->user_data;
	
	new_request = request;
	r_action    = NULL;
	have_after  = 0;
	ret         = 0;
	
	for(i=0, rule=data->rules; i<data->rules_count; i++, rule++){
		// filter before/after {{{
		if(rule->time == TIME_AFTER && pass == TIME_BEFORE)
			have_after = 1;
		if(rule->time != pass)
			continue;
		// }}}
		// filter requests {{{
		if(rule->filter != REQUEST_INVALID){
			if(r_action == NULL)
				r_action = hash_find_typed(request, TYPE_INT32, "action");
			
			if(!(HVALUE(r_action, request_actions) & rule->filter))
				continue;
		}
		// }}}
		
		data_t     *my_src        = NULL;
		data_t      my_src_data   = DATA_INVALID;
		data_ctx_t *my_src_ctx    = NULL;
		data_t     *my_dst        = NULL;
		data_ctx_t *my_dst_ctx    = NULL;
		data_t      my_dst_data   = DATA_INVALID;
		
		// read src {{{
		switch(rule->action){
			case VALUE_SET:
			case VALUE_LENGTH:
			case DATA_LENGTH:
			case DATA_ARITH:
			case DATA_CONVERT:
			case DATA_FREE:
				switch(rule->src_type){
					case THING_CONFIG:
						memcpy(&my_src_data, &rule->src_config, sizeof(rule->src_config));
						my_src     = &my_src_data;
						my_src_ctx = rule->src_config_ctx;
						break;
					
					case THING_KEY:
					case THING_REQUEST:
						if(rule->src_type == THING_KEY){
							my_src     = hash_get_data     (new_request, rule->src_key);
							my_src_ctx = hash_get_data_ctx (new_request, rule->src_key);
						}else{
							request_t **req_proto;
							if( (req_proto = find_proto(data, protos, rule->src_rule_num)) == NULL)
								goto parse_error;
							
							my_src     = hash_get_data     (*req_proto, rule->src_key);
							my_src_ctx = hash_get_data_ctx (*req_proto, rule->src_key); 
						}
						break;
						
					case THING_NOTHING:
						break;
				};
				if(my_src == NULL)
					goto parse_error;
				break;
			
			case DO_NOTHING:
			case VALUE_UNSET:
			case CALL_BACKEND:
		#ifdef DEBUG
			case HASH_DUMP:
		#endif
				break;
		}; // }}}
		// read dst {{{
		switch(rule->action){
			case VALUE_SET:
				if(rule->dst_info != 1)
					break;
				
				/* fallthrought */
			case DATA_ARITH:
			case DATA_CONVERT:
				switch(rule->dst_type){
					case THING_KEY:
					case THING_REQUEST:
						if(rule->dst_type == THING_KEY){
							my_dst     = hash_get_data     (new_request, rule->dst_key);
							my_dst_ctx = hash_get_data_ctx (new_request, rule->dst_key);
						}else{
							request_t **req_proto;
							if( (req_proto = find_proto(data, protos, rule->dst_rule_num)) == NULL)
								goto parse_error;
							
							my_dst     = hash_get_data     (*req_proto, rule->dst_key);
							my_dst_ctx = hash_get_data_ctx (*req_proto, rule->dst_key); 
						}
						break;
						
					case THING_CONFIG:
					case THING_NOTHING:
						break;
				};
				if(my_dst == NULL)
					goto parse_error;
				
				if(rule->copy){
					data_copy_local(&my_dst_data, my_dst);
					my_dst = &my_dst_data;
				}
				break;
			
			case DO_NOTHING:
			case VALUE_UNSET:
			case VALUE_LENGTH:
			case DATA_LENGTH:
			case CALL_BACKEND:
			case DATA_FREE:
		#ifdef DEBUG
			case HASH_DUMP:
		#endif
				break;
		}; // }}}
		// modify {{{
		switch(rule->action){
			case VALUE_UNSET:; // {{{
				data_t d_void = DATA_VOID;
				data_copy_local(&my_src_data, &d_void);
				my_src = &my_src_data;
				break; // }}}
			case VALUE_LENGTH:; // {{{
				data_t  my_vlength = DATA_PTR_SIZET(&my_src->data_size);
				data_copy_local(&my_src_data, &my_vlength);
				my_src = &my_src_data;
				break; // }}}
			
			case DATA_LENGTH:; // {{{
				size_t  my_dlength_mem = 0;
				data_t  my_dlength     = DATA_PTR_SIZET(&my_dlength_mem);
				
				my_dlength_mem = data_len(my_src, my_src_ctx);
				data_copy_local(&my_src_data, &my_dlength);
				my_src = &my_src_data;
				break; // }}}
			
			case DATA_ARITH:
				data_arithmetic(
					rule->operator,
					my_dst, my_dst_ctx,
					my_src, my_src_ctx
				);
				my_src = my_dst;
				break;
			case DATA_CONVERT:
				if(data_convert(my_dst, my_dst_ctx, my_src, my_src_ctx) != 0)
					goto parse_error;
				
				my_src = my_dst;
				break;
			case DATA_FREE:
				data_free(my_src);
				break;
			case CALL_BACKEND:;
				request_t **req_proto;
				if( (req_proto = find_proto(data, protos, i)) == NULL)
					goto parse_error;
				
				backend_query(rule->backend, *req_proto);
				break;
			
			case DO_NOTHING:
			case VALUE_SET:
				break;
		#ifdef DEBUG
			case HASH_DUMP:
				hash_dump(new_request);
				break;
		#endif
		};
		// }}}
		// write dst {{{
		switch(rule->action){
			case VALUE_SET:
			case VALUE_UNSET:
			case VALUE_LENGTH:
			case DATA_LENGTH:		
			case DATA_ARITH:
			case DATA_CONVERT:
				if(rule->src_info == 1 || rule->dst_info == 1){
					data_transfer(
						my_dst, my_dst_ctx,
						my_src, my_src_ctx
					);
				}else{
					switch(rule->dst_type){
						case THING_KEY:;
							hash_t  req_key[] = {
								{ rule->dst_key, *my_src },
								hash_next(request)
							};
							
							new_request = alloca(sizeof(req_key));
							memcpy(new_request, req_key, sizeof(req_key));
							break;
						
						case THING_REQUEST:;
							request_t **req_proto;
							
							if( (req_proto = find_proto(data, protos, rule->dst_rule_num)) == NULL)
								goto parse_error;
							
							hash_t  proto_key[] = {
								{ rule->dst_key, *my_src },
								hash_next(*req_proto)
							};
							
							*req_proto = alloca(sizeof(proto_key));
							memcpy(*req_proto, proto_key, sizeof(proto_key));
							break;
						default: // {{{
							break; // }}}
					};
				}
				break; // }}}
			case DO_NOTHING:
			case CALL_BACKEND:
			case DATA_FREE:
		#ifdef DEBUG
			case HASH_DUMP:
		#endif
				break;
		};
		// }}}
		continue;
	parse_error:
		if(rule->fatal == 1)
			return -EINVAL;
	}
	if(pass == 0){
		ret = chain_next_query(chain, new_request);
		
		if(have_after == 1){
			ssize_t rec_ret = rewrite_func_one(chain, new_request, TIME_AFTER, protos);
			if(rec_ret != 0)
				return rec_ret;
		}
	}
	return ret;
}

static ssize_t rewrite_func(chain_t *chain, request_t *request){ // {{{
	rewrite_user_data  *data      = (rewrite_user_data *)chain->user_data;
	request_t         **protos;
	size_t              protos_size;
	unsigned int        i;
	
	protos_size = data->rules_count * sizeof(request_t *);
	protos = alloca(protos_size);
	memset(protos, 0, protos_size);
	
	for(i=0; i<data->rules_count; i++){
		protos[i] = data->rules[i].request_proto;
	}
	
	return rewrite_func_one(chain, request, TIME_BEFORE, protos);
} // }}}

static chain_t chain_rewrite = {
	"rewrite",
	CHAIN_TYPE_CRWD,
	&rewrite_init,
	&rewrite_configure,
	&rewrite_destroy,
	{{
		.func_create = &rewrite_func,
		.func_set    = &rewrite_func,
		.func_get    = &rewrite_func,
		.func_delete = &rewrite_func,
		.func_move   = &rewrite_func,
		.func_count  = &rewrite_func,
		.func_custom = &rewrite_func
	}}
};
CHAIN_REGISTER(chain_rewrite)

