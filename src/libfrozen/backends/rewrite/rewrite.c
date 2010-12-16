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
 * 		script = DATA_STRING(" <some script> ");
 * 	}
 * @endcode
 */

/**
 * @ingroup mod_rewrite
 * @page page_rewrite_rules Rewrite script
 * 
 * Rewrite script consist of statements. All statements must end with ';'.
 * Possible statements:
 *      - variable define: 'var_t   variable_name;'
 *      - request define:  'request_t   subrequest;' 
 * 	- assignment:  'something = something;'
 * 	- function call: 'something = somefunc(param1, param2, param3);'
 *
 * User defined constants: (type)'string', where 'type' is data type, and 'string'
 * 	any string that can be converted to desired type (data_convert must exist)
 *  
 * Functions list:
 * 	- "backend((string)'backend_name', request)" - call backend
 * 		@param[in]  backend_name    Backend to call
 * 		@param[in]  request         Any request
 *      
 *      - "pass(request)" - pass request to underlying chain
 *      	@param[in]  request         Any request
 *
 * 	- "length(target)" - get length (data->data_size)
 * 		@param[in]  target   Target to measure
 *
 * 	- "data_length(target)"   - calc length of data_t. Output type is TYPE_SIZET
 * 		@param[in]  target   Target to measure
 *
 * 	- "data_arith((string)'+', dst, src)" - do arithmetic with targets. [ Dst = Dst (operation) Src ]
 * 		@param[in]  operation  Operation to do: '+' '-' '*' '/' [TYPE_STRING]
 * 		@param      dst_*      Destination target
 * 		@param[in]  src_*      Source target
 * 	- "data_alloca((string)'type', (size_t)'100')" - allocate new data
 * 		@param[in]  type       Data type to alloc
 * 		@param[in]  size       Size in units to alloc
 *
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
	temp_size = (data->script.variables_count) * sizeof(rewrite_variable_t);
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
		data_t           temp_any;
		
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
			case DATA_ALLOCA:
				// TODO this is dangerous! Rewrite with data_alloc, or something..
				to     = action->ret;
				param1 = action->params->list; // TODO error handling
				param2 = action->params->list->next;
				
				data_t      *type_data,     *size_data;
				data_ctx_t  *type_data_ctx, *size_data_ctx;
				
				rewrite_thing_get_data(&env, param1, &type_data, &type_data_ctx);
				rewrite_thing_get_data(&env, param2, &size_data, &size_data_ctx);
				
				data_type  new_type;
				if((new_type = data_type_from_string(data_value_ptr(type_data))) == TYPE_INVALID)
					return -EINVAL;
				
				size_t     new_size;
				data_read(size_data, size_data_ctx, 0, &new_size, sizeof(new_size));
				
				data_reinit(&temp_any, new_type, alloca(new_size), new_size);
				
				from_data     = &temp_any;
				from_data_ctx = NULL;
				break;
			case CALL_BACKEND:
				to     = action->ret;
				param1 = action->params->list;
				param2 = action->params->list->next;
				
				if(param1 == NULL || param1->type != THING_CONST)         return -EINVAL;
				if(param2 == NULL || param2->type != THING_ARRAY_REQUEST) return -EINVAL;
				
				rewrite_thing_get_data(&env, param1, &from_data, &from_data_ctx);
				
				hash_t find_backend[] = {
					{ NULL, *from_data }, // TODO ctx
					hash_end
				};
				
				backend_t *backend;
				if( (backend = backend_new(find_backend)) == NULL)
					return -EINVAL;
				
				temp_ret = backend_query(backend, env.requests[param2->id]);
				
				backend_destroy(backend);
				
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

