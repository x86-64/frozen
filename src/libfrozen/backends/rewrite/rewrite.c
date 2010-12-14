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
#include <backends/rewrite/rewrite.h>


typedef struct rewrite_user_data {
	rewrite_script_t  script;
} rewrite_user_data;
/* }}} */

/* init {{{ */
static int rewrite_init(chain_t *chain){ // {{{
	if( (chain->user_data = malloc(sizeof(rewrite_user_data))) == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
static int rewrite_destroy(chain_t *chain){ // {{{
	rewrite_user_data *data = (rewrite_user_data *)chain->user_data;
	
	rewrite_script_free(&data->script);
	
	free(data);
	chain->user_data = NULL;
	return 0;
} // }}}
static int rewrite_configure(chain_t *chain, hash_t *config){ // {{{
	data_t            *script;
	rewrite_user_data *data = (rewrite_user_data *)chain->user_data;
	
	if( (script = hash_get_typed_data(config, TYPE_STRING, "script")) == NULL)
		return_error(-EINVAL, "chain 'rewrite' parameter 'script' not supplied\n");
	
	if(rewrite_script_parse(&data->script, data_value_ptr(script)) != 0)
		return -EINVAL;
	
	return 0;
} // }}}
/* }}} */

/*
static request_t ** find_proto(rewrite_user_data *data, request_t **protos, unsigned int rule_num){
	if(rule_num == 0)
		return protos;
	
	if(rule_num >= data->rules_count)
		return NULL;
	
	return &protos[rule_num];
}
*/
	/*
static ssize_t rewrite_func_one(chain_t *chain, rewrite_script_t *script, request_t **requests){
	unsigned int        i;
	int                 have_after, ret_overriden;
	ssize_t             ret, ret2;
	//size_t              dst_size;
	//data_type           dst_type;
	hash_t             *r_action;
	request_t         **request;
	rewrite_rule_t     *rule;
	rewrite_user_data  *data      = (rewrite_user_data *)chain->user_data;
	
	r_action      = NULL;
	have_after    = 0;
	ret           = 0;
	ret_overriden = 0;
	request       = find_proto(data, protos, 0);
	
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
				r_action = hash_find_typed(*request, TYPE_INT32, "action");
			
			if(!(HVALUE(r_action, request_actions) & rule->filter))
				continue;
		}
		// }}}
		
		data_t     *my_src        = NULL;
		data_t      my_src_data   = DATA_VOID;
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
				switch(rule->src.type){
					case THING_CONFIG:
						my_src     = &rule->src.data;
						my_src_ctx =  rule->src.data_ctx;
						break;
					
					case THING_REQUEST:;
						request_t **req_proto;
						if( (req_proto = find_proto(data, protos, rule->src.rule_num)) == NULL)
							goto parse_error;
						
						my_src     = hash_get_data     (*req_proto, rule->src.subkey);
						my_src_ctx = hash_get_data_ctx (*req_proto, rule->src.subkey); 
						break;
						
					case THING_NOTHING:;
						my_src = &my_src_data;
						break;
				};
				if(my_src == NULL)
					goto parse_error;
				break;
			
			case DO_NOTHING:
			case DATA_ALLOCA:
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
				
			case DATA_ARITH:
			case DATA_CONVERT:
				switch(rule->dst.type){
					case THING_REQUEST:;
						request_t **req_proto;
						if( (req_proto = find_proto(data, protos, rule->dst.rule_num)) == NULL)
							goto parse_error;
						
						my_dst     = hash_get_data     (*req_proto, rule->dst.subkey);
						my_dst_ctx = hash_get_data_ctx (*req_proto, rule->dst.subkey); 
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
			case CALL_BACKEND:
			case DATA_LENGTH:
			case DATA_ALLOCA:
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
			case DATA_ALLOCA:;
				// TODO this is dangerous! Rewrite with data_alloc, or something..
				data_reinit(&my_src_data, rule->dst_key_type, alloca(rule->dst_key_size), rule->dst_key_size);
				my_src = &my_src_data;
				break;
			case DATA_FREE:
				data_free(my_src);
				break;
			case CALL_BACKEND:;
				request_t **req_proto;
				if( (req_proto = find_proto(data, protos, i+1)) == NULL)
					goto parse_error;
				
				ret2 = backend_query(rule->backend, *req_proto);
				if(rule->ret_override == 1){
					ret = ret2;
					ret_overriden = 1;
				}
				break;
			
			case DO_NOTHING:
			case VALUE_SET:
				break;
		#ifdef DEBUG
			case HASH_DUMP:
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
			case DATA_ALLOCA:
				if(rule->src_info == 1 || rule->dst_info == 1){
					data_transfer(
						my_dst, my_dst_ctx,
						my_src, my_src_ctx
					);
				}else{
					switch(rule->dst.type){
						case THING_REQUEST:;
							request_t **req_proto;
							
							if( (req_proto = find_proto(data, protos, rule->dst.rule_num)) == NULL)
								goto parse_error;
							
							hash_t  proto_key[] = {
								{ rule->dst.subkey, *my_src },
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
		ret2 = chain_next_query(chain, *request);
		if(ret_overriden == 0) ret = ret2;
		
		if(have_after == 1){
			if( (ret2 = rewrite_func_one(chain, TIME_AFTER, protos)) != 0)
				return ret2;
		}
	}
	return ret;
	return -1;
}
	*/

static void         rewrite_thing_get_data(rewrite_script_env_t *env, rewrite_thing_t *thing, data_t **data, data_ctx_t **data_ctx){ // {{{
	switch(thing->type){
		case THING_ARRAY_REQUEST_KEY:;
			request_t *request = env->requests[thing->id];
			
			*data     = hash_get_data     (request, thing->array_key);
			*data_ctx = hash_get_data_ctx (request, thing->array_key);
			break;
		
		case THING_CONST:;
			rewrite_variable_t *constant = &env->script->constants[thing->id];
			
			*data     = &constant->data;
			*data_ctx =  constant->data_ctx;
			break;
		
		case THING_VARIABLE:;
			rewrite_variable_t *variable = &env->variables[thing->id];
			
			*data     = &variable->data;
			*data_ctx =  variable->data_ctx;
			break;
		
		case THING_RET:;
			*data     = env->ret_data;
			*data_ctx = NULL;
			break;
		
		default:
			*data     = NULL;
			*data_ctx = NULL;
			return;
	};
} // }}}
static ssize_t rewrite_func(chain_t *chain, request_t *request){ // {{{
	unsigned int          action_id;
	size_t                temp_size;
	ssize_t               ret = 0;
	data_t                ret_data = DATA_PTR_SIZET(&ret);
	rewrite_script_env_t  env;
	rewrite_action_t     *action;
	rewrite_user_data    *data = (rewrite_user_data *)chain->user_data;
	rewrite_thing_t      *param1, *param2, *param3;
	
	/* if no actions - pass to next chain */
	if(data->script.actions_count == 0)
		return chain_next_query(chain, request);
	
	env.script   = &data->script;
	env.ret_data = &ret_data;
	
	/* alloc requests */
	temp_size = (data->script.requests_count + 1) * sizeof(request_t *);
	env.requests = alloca(temp_size);
	memset(env.requests, 0, temp_size);
	
	/* alloc variables */
	temp_size = (data->script.variables_count) * sizeof(rewrite_variable_t *);
	env.variables = alloca(temp_size);
	memset(env.variables, 0, temp_size);
	
	env.requests[0] = request;
	
	for(
		action_id = 0, action = data->script.actions;
		action_id < data->script.actions_count;
		action_id++, action++
	){
		rewrite_thing_t *to;
		data_t          *from_data;
		data_ctx_t      *from_data_ctx;
		size_t           temp_ret;
		data_t           temp_ret_data = DATA_PTR_SIZET(&temp_ret);
		
		switch(action->action){
			case VALUE_SET:
				to     = action->params->list;
				param1 = action->params->list->next;
				
				rewrite_thing_get_data(&env, param1, &from_data, &from_data_ctx);
				break;
			case CALL_PASS:
				to     = action->ret;
				param1 = action->params->list;
				
				if(param1 == NULL || param1->type != THING_ARRAY_REQUEST)
					return -EINVAL;
				
				temp_ret = chain_next_query(chain, env.requests[param1->id]);
				
				from_data     = &temp_ret_data;
				from_data_ctx = NULL;
				break;
			case VALUE_LENGTH:
				to     = action->ret;
				param1 = action->params->list;
				
				rewrite_thing_get_data(&env, param1, &from_data, &from_data_ctx);
				
				temp_ret = data_value_len(from_data);
				
				from_data     = &temp_ret_data;
				from_data_ctx = NULL;
				break;
			case DATA_LENGTH:
				to     = action->ret;
				param1 = action->params->list;
				
				rewrite_thing_get_data(&env, param1, &from_data, &from_data_ctx);
				
				temp_ret = data_len(from_data, from_data_ctx);
				
				from_data     = &temp_ret_data;
				from_data_ctx = NULL;
				break;
			case DATA_ARITH:
				to     = action->ret;
				param1 = action->params->list; // TODO error handling
				param2 = action->params->list->next;
				param3 = action->params->list->next->next;
				
				data_t      *dst_data,     *src_data;
				data_ctx_t  *dst_data_ctx, *src_data_ctx;
				
				rewrite_thing_get_data(&env, param1, &from_data, &from_data_ctx);
				rewrite_thing_get_data(&env, param2, &dst_data,  &dst_data_ctx);
				rewrite_thing_get_data(&env, param3, &src_data,  &src_data_ctx);
				
				if(data_value_type(from_data) != TYPE_STRING)
					return -EINVAL;
				
				char operator;
				data_read(from_data, from_data_ctx, 0, &operator, sizeof(operator));
				
				temp_ret = data_arithmetic(operator, dst_data, dst_data_ctx, src_data, src_data_ctx);
				
				from_data     = &temp_ret_data;
				from_data_ctx = NULL;
				break;
			default:
				return -ENOSYS;
		};
		
		switch(to->type){
			case THING_ARRAY_REQUEST_KEY:;
				request_t **request = &env.requests[to->id];
				
				hash_t  proto_key[] = {
					{ to->array_key, *from_data },
					// TODO ctx
					hash_next(*request)
				};
				
				*request = alloca(sizeof(proto_key));
				memcpy(*request, proto_key, sizeof(proto_key));
				break;
			case THING_RET:;
				data_read(from_data, from_data_ctx, 0, &ret, sizeof(ret));
				break;
			
			case THING_VARIABLE:;
				rewrite_variable_t *pass_var = &env.variables[to->id];
				
				data_copy_local(&pass_var->data, from_data);
				pass_var->data_ctx = NULL;
				break;
			default:
				break;
				
		};
	}
	
	return ret;
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

