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
 *      - variable define: 'var_t      variable_name;'
 *      - request define:  'request_t  subrequest;' 
 * 	- assignment:  'something = something;'
 * 	- function call: 'something = somefunc(param1, param2, param3);'
 * 	- if 'if(statement){};' NOTE ; after closing bracket
 * 	- negotiation 'if(!statement){};'
 *
 * User defined constants: (type)'string', where 'type' is data type, and 'string'
 * 	any string that can be converted to desired type (data_convert must exist)
 *  
 * Functions list:
 * 	- "backend((string)'backend_name', request)" - call backend
 * 		@param[in]  backend_name    Backend to call
 * 		@param[in]  request         Any request
 *      
 *      - "pass(request)" - pass request to underlying backend
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
 *
 * 	- "data_alloca((string)'type', (size_t)'100')" - allocate new data
 * 		@param[in]  type       Data type to alloc
 * 		@param[in]  size       Size in units to alloc
 *
 */

#include <libfrozen.h>
#include <alloca.h>
#include <rewrite.h>

#define EMODULE 12
typedef struct rewrite_userdata {
	unsigned int      inited;
	rewrite_script_t  script;
} rewrite_userdata;
/* }}} */

/* init {{{ */
static int rewrite_init(backend_t *backend){ // {{{
	if( (backend->userdata = malloc(sizeof(rewrite_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int rewrite_destroy(backend_t *backend){ // {{{
	rewrite_userdata *data = (rewrite_userdata *)backend->userdata;
	
	if(data->inited == 1){
		rewrite_script_free(&data->script);
	}
	free(data);
	return 0;
} // }}}
static int rewrite_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t            ret;
	char              *script;
	rewrite_userdata  *data = (rewrite_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRING, script, config, HK(script));
	if(ret != 0)
		return error("script not supplied");
	
	if(rewrite_script_parse(&data->script, script) != 0)
		return error("script parse failed");
	
	data->inited = 1;
	return 0;
} // }}}
/* }}} */

static void         rewrite_thing_get_data(rewrite_script_env_t *env, rewrite_thing_t *thing, data_t **data, data_ctx_t **data_ctx){ // {{{
	switch(thing->type){
		case THING_ARRAY_REQUEST_KEY:;
			request_t *request = env->requests[thing->id];
			
			hash_data_find(request, thing->array_key, data, data_ctx);
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

typedef struct rewrite_stack_frame_t rewrite_stack_frame_t;
struct rewrite_stack_frame_t {
	rewrite_action_block_t  *ablock;
	rewrite_action_t        *action;
	unsigned int             action_id;
};
#define ablock_call(block) do { frame->action = action + 1; frame->action_id = action_id + 1; ablock = block; goto ablock_enter; }while(0);

static ssize_t rewrite_func_ablock(rewrite_script_env_t *env, rewrite_action_block_t *ablock){ // {{{
	unsigned int           action_id;
	ssize_t                ret = 0;
	data_t                 ret_data = DATA_PTR_SIZET(&ret);
	rewrite_thing_t       *param1, *param2, *param3;
	rewrite_action_t      *action;
	rewrite_stack_frame_t *stack, *frame;
	unsigned int           frame_id, stack_items;
	
	env->ret_data = &ret_data;
	stack         = NULL;
	stack_items   = 0;
	
ablock_enter:
	frame_id = stack_items++;
	stack = realloc(stack, stack_items * sizeof(rewrite_stack_frame_t));
	frame = &stack[frame_id];
	frame->action_id = 0;
	frame->action    = ablock->actions;
	frame->ablock    = ablock;
	
ablock_continue:
	for(
		action_id = frame->action_id, action = frame->action;
		action_id < ablock->actions_count;
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
				
				rewrite_thing_get_data(env, param1, &from_data, &from_data_ctx);
				break;
			case VALUE_CMP:
				param1 = action->params->list;
				param2 = action->params->list->next;
				
				rewrite_thing_get_data(env, param1, &from_data, &from_data_ctx);
				
				// TODO call data_is_null
				unsigned int cmp_res = 0;
				switch(from_data->type){
					case TYPE_SIZET:;
					case TYPE_UINT32T:;
						unsigned int m_i32 = *(unsigned int *)(from_data->data_ptr);
						cmp_res = (m_i32 == 0) ? 0 : 1;
						break;
					default:
						ret = error("no cmp");
						goto exit;
				};
				
				if(cmp_res == 1){
					ablock_call(param2->block);
				}			
				break;
			case VALUE_NEG:
				to     = action->params->list;
				param1 = action->params->list->next;
				
				rewrite_thing_get_data(env, param1, &from_data, &from_data_ctx);
				
				// TODO call data_is_null
				unsigned int cmp_res2 = 0;
				switch(from_data->type){
					case TYPE_SIZET:;
					case TYPE_UINT32T:;
						unsigned int m_i32 = *(unsigned int *)(from_data->data_ptr);
						cmp_res2 = (m_i32 == 0) ? 0 : 1;
						break;
					default:
						ret = error("no cmp");
						goto exit;
				};
				
				temp_ret      = (cmp_res2 == 0) ? 1 : 0;
				from_data     = &temp_ret_data;
				from_data_ctx = NULL;
				break;
			case CALL_PASS:
				to     = action->ret;
				param1 = action->params->list;
				
				if(param1 == NULL || param1->type != THING_ARRAY_REQUEST){
					ret = error("pass failed");
					goto exit;
				}
				
				temp_ret = backend_pass(env->backend, env->requests[param1->id]);
				
				from_data     = &temp_ret_data;
				from_data_ctx = NULL;
				break;
			case VALUE_LENGTH:
				to     = action->ret;
				param1 = action->params->list;
				
				rewrite_thing_get_data(env, param1, &from_data, &from_data_ctx);
				
				temp_ret = data_value_len(from_data);
				
				from_data     = &temp_ret_data;
				from_data_ctx = NULL;
				break;
			case DATA_LENGTH:
				to     = action->ret;
				param1 = action->params->list;
				
				rewrite_thing_get_data(env, param1, &from_data, &from_data_ctx);
				
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
				
				rewrite_thing_get_data(env, param1, &from_data, &from_data_ctx);
				rewrite_thing_get_data(env, param2, &dst_data,  &dst_data_ctx);
				rewrite_thing_get_data(env, param3, &src_data,  &src_data_ctx);
				
				if(data_value_type(from_data) != TYPE_STRING){
					ret = error("arithmetic failed");
					goto exit;
				}
				
				char operator;
				data_read(from_data, from_data_ctx, 0, &operator, sizeof(operator));
				
				temp_ret = data_arithmetic(operator, dst_data, dst_data_ctx, src_data, src_data_ctx);
				
				from_data     = &temp_ret_data;
				from_data_ctx = NULL;
				break;
			case DATA_CMP:
				to     = action->ret;
				param1 = action->params->list; // TODO error handling
				param2 = action->params->list->next;
				
				data_t      *data1,     *data2;
				data_ctx_t  *data1_ctx, *data2_ctx;
				
				rewrite_thing_get_data(env, param1, &data1,  &data1_ctx);
				rewrite_thing_get_data(env, param2, &data2,  &data2_ctx);
				
				temp_ret = data_cmp(data1, data1_ctx, data2, data2_ctx);
				
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
				
				rewrite_thing_get_data(env, param1, &type_data, &type_data_ctx);
				rewrite_thing_get_data(env, param2, &size_data, &size_data_ctx);
				
				data_type  new_type;
				if((new_type = data_type_from_string(data_value_ptr(type_data))) == TYPE_INVALID){
					ret = error("alloca data invalid");
					goto exit;
				}
				
				size_t     new_size;
				data_read(size_data, size_data_ctx, 0, &new_size, sizeof(new_size));
				
				data_alloc_local(&temp_any, new_type, new_size);
				
				from_data     = &temp_any;
				from_data_ctx = NULL;
				break;
			case CALL_BACKEND:
				to     = action->ret;
				param1 = action->params->list;
				param2 = action->params->list->next;
				
				if(param1 == NULL || param1->type != THING_CONST){         ret = -EINVAL; goto exit; }
				if(param2 == NULL || param2->type != THING_ARRAY_REQUEST){ ret = -EINVAL; goto exit; }
				
				rewrite_thing_get_data(env, param1, &from_data, &from_data_ctx);
				
				// TODO SEC insecure
				if(data_value_type(from_data) != TYPE_STRING){ ret = -EINVAL; goto exit; }
				
				backend_t *backend;
				if( (backend = backend_acquire(data_value_ptr(from_data))) == NULL){ // TODO ctx
					ret = error("backend_acquire failed");
					goto exit;
				}
				
				temp_ret = backend_query(backend, env->requests[param2->id]);
				
				backend_destroy(backend);
				
				from_data     = &temp_ret_data;
				from_data_ctx = NULL;
				break;
			default:
				ret = -ENOSYS;
				goto exit;
		};
		
		switch(to->type){
			case THING_ARRAY_REQUEST_KEY:;
				if(from_data == NULL){
					data_alloc_local(&temp_any, TYPE_VOID, 0);
					from_data = &temp_any;
				}
				request_t **request = &env->requests[to->id];
				
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
				rewrite_variable_t *pass_var = &env->variables[to->id];
				
				//data_copy_local(&pass_var->data, from_data);
				memcpy(&pass_var->data, from_data, sizeof(data_t));
				pass_var->data_ctx = NULL;
				break;
			default:
				break;
				
		};
	}
//ablock_leave:
	frame_id = --stack_items - 1;
	if(stack_items == 0)
		goto exit;
	
	stack  = realloc(stack, stack_items * sizeof(rewrite_stack_frame_t));
	frame  = &stack[frame_id];
	ablock = frame->ablock;
	goto ablock_continue;
exit:
	free(stack);
	return ret;
} // }}}
static ssize_t rewrite_func(backend_t *backend, request_t *request){ // {{{
	size_t                temp_size;
	rewrite_script_env_t  env;
	rewrite_userdata    *data = (rewrite_userdata *)backend->userdata;
	
	/* if no actions - pass to next backend */
	if(data->script.main->actions_count == 0)
		return backend_pass(backend, request);
	
	env.script   = &data->script;
	env.backend    = backend;
	
	/* alloc requests */
	temp_size = (data->script.requests_count + 1) * sizeof(request_t *);
	env.requests = alloca(temp_size);
	memset(env.requests, 0, temp_size);
	
	/* alloc variables */
	temp_size = (data->script.variables_count) * sizeof(rewrite_variable_t);
	env.variables = alloca(temp_size);
	memset(env.variables, 0, temp_size);
	
	env.requests[0] = request;
	
	return rewrite_func_ablock(&env, data->script.main);
} // }}}

backend_t backend_rewrite = {
	"rewrite",
	.supported_api = API_CRWD,
	.func_init      = &rewrite_init,
	.func_configure = &rewrite_configure,
	.func_destroy   = &rewrite_destroy,
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


