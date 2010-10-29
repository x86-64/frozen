#include <libfrozen.h>
#include <alloca.h>

typedef enum rewrite_actions {
	VALUE_SET,
	VALUE_DELETE,
	CALC_LENGTH,
	CALC_DATA_LENGTH,
	CALL_BACKEND,
	CALL_ARITH
} rewrite_actions;

typedef enum things {
	THING_NOTHING,
	THING_CONFIG,
	THING_KEY,
	THING_BUFFER
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
		char         *src_key_name;
		off_t         src_buffer_off;
		size_t        src_buffer_len;
		data_type     src_config_type;
		void         *src_config_data;
		size_t        src_config_size;
		
	// destination
		things        dst_type;
		char         *dst_key_name;
		data_type     dst_key_type;
		size_t        dst_key_size;
		off_t         dst_buffer_off;
		size_t        dst_buffer_len;
		
	// rest
		data_ctx_t    data_ctx;
		unsigned int  copy;
		unsigned int  fatal;
		backend_t    *backend;
} rewrite_rule_t;

typedef struct rewrite_user_data {
	rewrite_rule_t   *rules;
	unsigned int      rules_count;
} rewrite_user_data;

static rewrite_actions  rewrite_str_to_action(char *string){ // {{{
	if(strcasecmp(string, "set")            == 0) return VALUE_SET;
	if(strcasecmp(string, "unset")          == 0) return VALUE_DELETE;
	if(strcasecmp(string, "length")         == 0) return CALC_LENGTH;
	if(strcasecmp(string, "data_length")    == 0) return CALC_DATA_LENGTH;
	if(strcasecmp(string, "backend")        == 0) return CALL_BACKEND;
	if(strcasecmp(string, "arith")          == 0) return CALL_ARITH;
	return -1;
} // }}}

static void rewrite_rule_free(rewrite_rule_t *rule){ // {{{
	if(rule->src_config_data != NULL)
		free(rule->src_config_data);
	if(rule->src_key_name != NULL)
		free(rule->src_key_name);
	if(rule->dst_key_name != NULL)
		free(rule->dst_key_name);
} // }}}

/*!
 * Rewrite rule can contain action, source and destination targets and optional parameters.
 * Source and destination targets defined as follows:
 * @code
 * 	{ src_key    = "source_key"                                     }  // for src key
 * 	{ src_config = "config_key"                                     }  // for config key
 * 	{ dst_key    = "target_key", dst_type = "binary", dst_size = 10 }  // for dst key
 * 	{ dst_same   = 1                                                }  // for dst key eq src
 * 	{ src_buffer = 1                                                }  // for src buffer
 * 	{ src_buffer = 1, src_buffer_off = 0                            }  // for src buffer
 * 	{ src_buffer = 1, src_buffer_off = 0, src_buffer_len = 10       }  // for src buffer
 * 	{ dst_buffer = 1, dst_buffer_off = 0, dst_buffer_len = 10       }  // for dst buffer
 * @endcode
 * dst_type and dst_size is optional then source target can provide these values.
 *  
 *
 * Action can be one of following:
 * @code
 * 	set           // set key or buffer value
 * 	unset         // unset key or buffer
 * 	length        // get length of source (key->value_size and buffer->size)
 * 	data_length   // calc length of data_t 
 * 	arith         // do arithmetic with target
 * 	backend       // call backend
 * @endcode
 *
 * Other parameters:
 * - data_ctx  - context for buffer manipulations
 * - value     - value for key 'set' operation
 * - operation - '+' '-' '*' '/'
 * - operand   - int32 value
 * - before    - do request before chain_next (default)
 * - after     - do request after  chain_next
 * - fatal     - return error on failed rule
 */
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
		label_error(cleanup, "rewrite rule 'action' not supplied\n");
	if( (new_rule.action = rewrite_str_to_action(temp)) == -1)
		label_error(cleanup, "rewrite rule 'action' invalid\n");
	// }}}
	// parse filter {{{
	new_rule.filter =
		( (htemp = hash_find_typed(config, TYPE_INT32, "on-request")) != NULL) ?
		HVALUE(htemp, unsigned int) : -1;
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
	do{ // src parse {{{
		if( hash_get_typed(config, TYPE_STRING, "src_key", &temp, NULL) == 0){
			new_rule.src_type     = THING_KEY;
			new_rule.src_key_name = strdup(temp); // TODO memory leak here
			break;
		}
		if( hash_get_typed(config, TYPE_STRING, "src_config", &temp, NULL) == 0){
			new_rule.src_type        = THING_CONFIG;
			if( (htemp = hash_find(config, (char *)temp)) == NULL)
				label_error(cleanup, "rewrite rule 'src_config' have wrong parameter\n");
			
			new_rule.src_config_type = hash_get_data_type(htemp);
			new_rule.src_config_size = hash_get_value_size(htemp);
			new_rule.src_config_data = malloc(new_rule.src_config_size); // TODO memory leak here
			memcpy(
				new_rule.src_config_data,
				hash_get_value_ptr(htemp),
				new_rule.src_config_size
			);
			break;
		}
		if( hash_find(config, "src_buffer") != NULL){
			new_rule.src_type       = THING_BUFFER;
			new_rule.src_buffer_off =
				( (htemp = hash_find_typed(config, TYPE_OFFT, "src_buffer_off")) != NULL) ?
				HVALUE(htemp, off_t) : 0;
			new_rule.src_buffer_len = 
				( (htemp = hash_find_typed(config, TYPE_SIZET, "src_buffer_len")) != NULL) ?
				HVALUE(htemp, size_t) : (size_t)-1;
			break;
		}
	}while(0); // src parse }}}
	do{ // dst parse {{{
		if( hash_get_typed(config, TYPE_STRING, "dst_key", &temp, NULL) == 0){
			new_rule.dst_type     = THING_KEY;
			new_rule.dst_key_name = strdup(temp); // TODO memory leak here
			new_rule.dst_key_type =
				( (htemp = hash_find_typed(config, TYPE_STRING, "dst_type")) != NULL) ?
				data_type_from_string((char *)htemp->value) : -1;
			new_rule.dst_key_size =
				( (htemp = hash_find_typed(config, TYPE_SIZET, "dst_size")) != NULL) ?
				HVALUE(htemp, size_t) : 0;
			break;
		}
		if( hash_find(config, "dst_buffer") != NULL){
			new_rule.dst_type       = THING_BUFFER;
			new_rule.dst_buffer_off =
				( (htemp = hash_find_typed(config, TYPE_OFFT, "dst_buffer_off")) != NULL) ?
				HVALUE(htemp, off_t) : 0;
			new_rule.dst_buffer_len = 
				( (htemp = hash_find_typed(config, TYPE_SIZET, "dst_buffer_len")) != NULL) ?
				HVALUE(htemp, size_t) : (size_t)-1;
			break;
		}
	}while(0); // dst parse }}}
	
	/*if( (r_backend = hash_find(config, "backend")) == NULL)
		return_error(-EINVAL, "chain 'rewrite' parameter 'backend' not supplied\n");
	if( (data->modify_backend = backend_new(r_backend)) == NULL)
		return_error(-EINVAL, "chain 'rewrite' parameter 'backend' invalid\n");*/
	
	// check parameters {{{
	switch(new_rule.action){
		case VALUE_SET:
			if(new_rule.src_type == THING_NOTHING)
				label_error(cleanup, "rewrite rule missing src target\n");
			if(new_rule.dst_type == THING_NOTHING)
				label_error(cleanup, "rewrite rule missing dst target\n");
			if(new_rule.dst_type == THING_KEY){
				if(
					new_rule.src_type == THING_BUFFER &&
					( new_rule.dst_key_type == -1 || new_rule.dst_key_size == 0)
				)
					label_error(cleanup, "rewrite rule can't fill dst key. supply dst_type and dst_size\n");
			}
			break;
		case VALUE_DELETE:
			if(new_rule.dst_type == THING_NOTHING)
				label_error(cleanup, "rewrite rule missing dst target\n");
			break;
		default:
			return -EINVAL;
	};
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

/**
 * Accepted configuration:
 * @code
 * 	settings = {
 * 		rules = {               // required rules list
 *                   <rewrite rules>
 * 		}
 * 	}
 * @endcode
 */
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

static ssize_t rewrite_func_one(chain_t *chain, request_t *request, buffer_t *buffer, int pass){
	unsigned int        i;
	int                 have_after;
	ssize_t             ret;
	size_t              dst_size, cpy_size;
	data_type           dst_type;
	hash_t             *r_action, *r_src_key;
	request_t          *new_request;
	buffer_t           *new_buffer;
	rewrite_rule_t     *rule;
	rewrite_user_data  *data      = (rewrite_user_data *)chain->user_data;
	
	new_request = request;
	new_buffer  = buffer;
	r_action    = NULL;
	have_after  = 0;
	ret         = 0;
	
	for(i=0, rule=data->rules; i<data->rules_count; i++){
		// filter requests {{{
		if(rule->filter != -1){
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
		
		switch(rule->action){
		case VALUE_SET: // {{{
			switch(rule->dst_type){
			case THING_BUFFER: // {{{
				switch(rule->src_type){
				case THING_BUFFER: // {{{
					cpy_size = buffer_get_size(buffer);
					cpy_size = 
						(rule->src_buffer_len == -1) ?
						cpy_size : MIN(cpy_size, rule->src_buffer_len);
					cpy_size =
						(rule->dst_buffer_len == -1) ?
						cpy_size : MIN(cpy_size, rule->dst_buffer_len);
					
					buffer_memcpy(
						buffer, rule->dst_buffer_off,
						buffer, rule->src_buffer_off,
						cpy_size
					);
					break; // }}}
				case THING_KEY: // {{{
					r_src_key = hash_find(request, rule->src_key_name);
					if(r_src_key == NULL){
						if(rule->fatal == 1) return -EINVAL;
						continue;
					}
					cpy_size  = hash_get_value_size(r_src_key);
					cpy_size  =
						(rule->dst_buffer_len == -1) ?
						cpy_size : MIN(cpy_size, rule->dst_buffer_len);
					
					buffer_write(
						buffer, rule->dst_buffer_off,
						hash_get_value_ptr(r_src_key), cpy_size
					);
					break; // }}}
				case THING_CONFIG: // {{{
					cpy_size  = rule->src_config_size;
					cpy_size  =
						(rule->dst_buffer_len == -1) ?
						cpy_size : MIN(cpy_size, rule->dst_buffer_len);
					buffer_write(
						buffer, rule->dst_buffer_off,
						rule->src_config_data, cpy_size
					);
					break; // }}}
				default: // {{{
					return -EFAULT;
				}; // }}}
				break; // }}}
			case THING_KEY: // {{{
				switch(rule->src_type){
				case THING_KEY: // {{{
					r_src_key = hash_find(request, rule->src_key_name);
					if(r_src_key == NULL){
						if(rule->fatal == 1) return -EINVAL;
						continue;
					}
					
					dst_type = hash_get_data_type(r_src_key);
					dst_type = (rule->dst_key_type != -1) ? rule->dst_key_type : dst_type;
					dst_size = hash_get_value_size(r_src_key);
					dst_size = (rule->dst_key_size != -1) ? MIN(rule->dst_key_size, dst_size) : dst_size;
					
					hash_t  key_to_key_req[] = {
						{ rule->dst_key_name, dst_type, hash_get_value_ptr(r_src_key), dst_size },
						hash_next(request)
					};
					new_request = alloca(sizeof(key_to_key_req));
					memcpy(new_request, key_to_key_req, sizeof(key_to_key_req));
					break; // }}}
				case THING_CONFIG: // {{{
					dst_type = rule->src_config_type;
					dst_type = (rule->dst_key_type != -1) ? rule->dst_key_type : dst_type;
					dst_size = rule->src_config_size;
					dst_size = (rule->dst_key_size != -1) ? MIN(rule->dst_key_size, dst_size) : dst_size;
					
					hash_t  config_to_key_req[] = {
						{ rule->dst_key_name, dst_type, rule->src_config_data, dst_size },
						hash_next(request)
					};
					new_request = alloca(sizeof(config_to_key_req));
					memcpy(new_request, config_to_key_req, sizeof(config_to_key_req));
					break; // }}}
				case THING_BUFFER: // {{{
					// TODO
					break; // }}}
				default: // {{{
					break; // }}}
				};
				break; // }}}
			default: // {{{
				break; // }}}
			};
			break; // }}}
		case VALUE_DELETE: // {{{
			switch(rule->dst_type){
				case THING_BUFFER: new_buffer = NULL; break;
				case THING_KEY:;
					hash_t  del_request[] = {
						{ rule->dst_key_name, DATA_VOID },
						hash_next(request)
					};
					new_request = alloca(sizeof(del_request));
					memcpy(new_request, del_request, sizeof(del_request));
					break;
				default:
					break;
			};
			break;
		// }}}
		default: // {{{
			return -EINVAL;
		// }}}
		};
	}
	if(pass == 0){
		ret = chain_next_query(chain, new_request, new_buffer);
		
		if(have_after == 1){
			ssize_t rec_ret = rewrite_func_one(chain, request, buffer, 1);
			if(rec_ret != 0)
				return rec_ret;
		}
	}
	return ret;
}

static ssize_t rewrite_func(chain_t *chain, request_t *request, buffer_t *buffer){ // {{{
	return rewrite_func_one(chain, request, buffer, 0);
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

