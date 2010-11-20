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
		request_t    *request_curr;
		
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
	return DO_NOTHING;
} // }}}

static void rewrite_rule_free(rewrite_rule_t *rule){ // {{{
	if(rule->src_key != NULL) free(rule->src_key);
	if(rule->dst_key != NULL) free(rule->dst_key);
	
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
	new_rule.request_curr  = new_rule.request_proto;
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

static rewrite_rule_t *find_rule(rewrite_user_data *data, unsigned int rule_num){
	if(rule_num >= data->rules_count)
		return NULL;
	
	return &data->rules[rule_num];
}

static ssize_t rewrite_func_one(chain_t *chain, request_t *request, int pass){
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
		// filter requests {{{
		if(rule->filter != REQUEST_INVALID){
			if(r_action == NULL)
				r_action = hash_find_typed(request, TYPE_INT32, "action");
			
			if(HVALUE(r_action, request_actions) != rule->filter)
				continue;
		}
		// }}}
		// filter before/after {{{
		if(rule->time == TIME_AFTER && pass == 0){
			have_after = 1;
			continue;
		}
		// }}}
		
		data_t     *my_src        = NULL;
		data_t      my_src_data   = DATA_INVALID;
		data_ctx_t *my_src_ctx    = NULL;
		data_t     *my_dst        = NULL;
		data_ctx_t *my_dst_ctx    = NULL;
		
		// read src {{{
		switch(rule->action){
			case VALUE_SET:
			case VALUE_LENGTH:
			case DATA_LENGTH:
			case DATA_ARITH:
				switch(rule->src_type){
					case THING_CONFIG:
						memcpy(&my_src_data, &rule->src_config, sizeof(rule->src_config));
						my_src     = &my_src_data;
						my_src_ctx = rule->src_config_ctx;
						break;
					
					case THING_KEY:
					case THING_REQUEST:
						if(rule->src_type == THING_KEY){
							my_src     = hash_get_data     (request, rule->src_key);
							my_src_ctx = hash_get_data_ctx (request, rule->src_key);
						}else{
							rewrite_rule_t *req_rule;
							if( (req_rule = find_rule(data, rule->src_rule_num)) == NULL){
								if(rule->fatal == 1) return -EINVAL; else break;
							}
							my_src     = hash_get_data     (req_rule->request_curr, rule->src_key);
							my_src_ctx = hash_get_data_ctx (req_rule->request_curr, rule->src_key); 
						}
						if(my_src == NULL){
							if(rule->fatal == 1) return -EINVAL;
							break;
						}
						if(rule->copy){
							data_copy_local(&my_src_data, my_src);
							my_src = &my_src_data;
						}
						break;
					
					case THING_NOTHING:
						break;
				};
				if(my_src == NULL){
					if(rule->fatal == 1) return -EINVAL; else break;
				}
				break;
			
			case DO_NOTHING:
			case VALUE_UNSET:
			case CALL_BACKEND:
				break;
		}; // }}}
		// TODO read dst
		// modify {{{
		switch(rule->action){
			case VALUE_UNSET: // {{{
				switch(rule->dst_type){
					case THING_KEY:;
						hash_t  del_req_key[] = {
							{ rule->dst_key, DATA_VOID },
							hash_next(request)
						};
						
						new_request = alloca(sizeof(del_req_key));
						memcpy(new_request, del_req_key, sizeof(del_req_key));
						break;
					
					case THING_REQUEST:;
						rewrite_rule_t *req_rule;
						
						if( (req_rule = find_rule(data, rule->dst_rule_num)) == NULL){
							if(rule->fatal == 1) return -EINVAL; else break;
						}
						
						hash_t  del_proto_key[] = {
							{ rule->dst_key, DATA_VOID },
							hash_next(req_rule->request_curr)
						};
						
						req_rule->request_curr = alloca(sizeof(del_proto_key));
						memcpy(req_rule->request_curr, del_proto_key, sizeof(del_proto_key));
						break;
					
					case THING_CONFIG:
						return -EINVAL;
					
					case THING_NOTHING:
						break;
				};
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
				break;
			
			case CALL_BACKEND:
				backend_query(rule->backend, rule->request_curr);
				break;
			
			case DO_NOTHING:
			case VALUE_SET:
				break;
		};
		// }}}
		
	/*
		// write dst {{{
		switch(rule->action){
			case VALUE_SET:
			case CALC_LENGTH:
			case CALC_DATA_LENGTH:
			case CALL_ARITH:
			case CALL_BACKEND: // {{{
				switch(rule->dst_type){
					case THING_KEY:
					case THING_REQUEST_KEY:; // {{{
						void *new_ptr;
						dst_type = (rule->dst_key_type != TYPE_INVALID) ? rule->dst_key_type : my_key_type;
						dst_size = (rule->dst_key_size != -1) ? MIN(rule->dst_key_size, my_key_size) : my_key_size;
						
						switch(my_thing){
							case THING_KEY:    new_ptr = my_key_ptr; break; 
							case THING_BUFFER: new_ptr = buffer_defragment(my_buffer); break;
							default: return -EFAULT;
						};
						hash_t  key_to_key_req[] = {
							{ rule->dst_key_name, dst_type, new_ptr, dst_size },
							hash_next(request)
						};
						if(rule->dst_type == THING_KEY){
							new_request = alloca(sizeof(key_to_key_req));
							memcpy(new_request, key_to_key_req, sizeof(key_to_key_req));
						}else{
							rewrite_rule_t *req_rule;
							if(rule->dst_rule_num >= data->rules_count){
								if(rule->fatal == 1) return -EINVAL;
								break;
							}
							req_rule  = &data->rules[rule->dst_rule_num];
							req_rule->request_curr = alloca(sizeof(key_to_key_req));
							memcpy(req_rule->request_curr, key_to_key_req, sizeof(key_to_key_req));
						}
						break; // }}}
					default: // {{{
						break; // }}}
				};
				break; // }}}
			case VALUE_DELETE: // {{{
				break; // }}}
			default: // {{{
				return -EINVAL;
			// }}}
		};
		// }}}
*/
	}
	if(pass == 0){
		ret = chain_next_query(chain, new_request);
		
		if(have_after == 1){
			ssize_t rec_ret = rewrite_func_one(chain, request, 1);
			if(rec_ret != 0)
				return rec_ret;
		}
	}
	return ret;
}

static ssize_t rewrite_func(chain_t *chain, request_t *request){ // {{{
	return rewrite_func_one(chain, request, 0);
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

