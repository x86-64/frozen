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
 * 		@param[in]  operation  Operation to do: '+' '-' '*' '/' [TYPE_STRINGT]
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
/* }} */

/* init {{ */
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
	
	hash_data_copy(ret, TYPE_STRINGT, script, config, HK(script));
	if(ret != 0)
		return error("script not supplied");
	
	if(rewrite_script_parse(&data->script, script) != 0)
		return error("script parse failed");
	
	data->inited = 1;
	return 0;
} // }}}
/* }} */

static data_t * rewrite_thing_get_data(rewrite_script_env_t *env, rewrite_thing_t *thing){ // {{{
	switch(thing->type){
		case THING_HASH_ELEMENT:;
			request_t *request = env->requests[thing->id];
			return hash_data_find(request, thing->array_key);
		
		case THING_CONST:    return &(env->script->constants[thing->id].data);
		case THING_VARIABLE: return &(env->variables[thing->id].data);
		case THING_RET:      return env->ret_data;
		default:             break;
	};
	return NULL;
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
	ssize_t                ret = -EEXIST;
	data_t                 ret_data = DATA_PTR_SIZET(&ret);
	rewrite_thing_t       *param1, *param2;
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
		data_t          *from;
		size_t           action_ret;
		data_t           action_ret_data = DATA_PTR_SIZET(&action_ret);
		data_t           temp_any;
		
		switch(action->action){
			case LANG_SET:
				to     = action->params->list;
				param1 = action->params->list->next;
				
				from   = rewrite_thing_get_data(env, param1);
				break;
			
			case LANG_IF:
				param1 = action->params->list;
				param2 = action->params->list->next;
				
				if(param1 == NULL){                                       ret = -EINVAL; goto exit; }
				if(param2 == NULL || param2->type != THING_ACTION_BLOCK){ ret = -EINVAL; goto exit; }
				
				from = rewrite_thing_get_data(env, param1);
				
				fastcall_is_null r_isnull = { { 3, ACTION_IS_NULL } };
				if(data_query(from, &r_isnull) == 0 && r_isnull.is_null == 0){
					ablock_call(param2->block);
				}
				break;
			
			case BACKEND_QUERY:
				to     = action->ret;
				param1 = action->params->list;
				param2 = action->params->list->next;
				
				if(param1 == NULL || param1->type != THING_CONST){ ret = -EINVAL; goto exit; }
				if(param2 == NULL || param2->type != THING_HASHT){ ret = -EINVAL; goto exit; }
				
				from = rewrite_thing_get_data(env, param1);
				
				if(from->type != TYPE_BACKENDT){ ret = -EINVAL; goto exit; }
				
				action_ret = backend_query(from->ptr, env->requests[param2->id]);
				
				from   = &action_ret_data;
				break;
				
			case BACKEND_PASS:
				to     = action->ret;
				param1 = action->params->list;
				
				if(param1 == NULL || param1->type != THING_HASHT){ ret = error("pass failed"); goto exit; }
				
				action_ret = ( (action_ret = backend_pass(env->backend, env->requests[param1->id])) < 0) ? action_ret : -EEXIST;
				
				from   = &action_ret_data;
				break;
			
			case DATA_QUERY:
				break;
			default:
				ret = -ENOSYS;
				goto exit;
		};
		
		if(from == NULL){
			temp_any.type = TYPE_VOIDT;
			temp_any.ptr  = NULL;
			from          = &temp_any;
		}
		
		switch(to->type){
			case THING_HASH_ELEMENT:;
				request_t **request = &env->requests[to->id];
				
				hash_t  proto_key[] = {
					{ to->array_key, *from },
					hash_next(*request)
				};
				
				*request = alloca(sizeof(proto_key));
				memcpy(*request, proto_key, sizeof(proto_key));
				break;
			case THING_RET:;
				fastcall_read r_read = { { 5, ACTION_READ }, 0, &ret, sizeof(ret) };
				data_query(from, &r_read);
				break;
			
			case THING_VARIABLE:;
				rewrite_variable_t *pass_var = &env->variables[to->id];
				memcpy(&pass_var->data, from, sizeof(data_t));
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
	size_t                 temp_size;
	rewrite_script_env_t   env;
	rewrite_userdata      *data              = (rewrite_userdata *)backend->userdata;
	
	/* if no actions - pass to next backend */
	if(data->script.main->actions_count == 0)
		return backend_pass(backend, request);
	
	env.script    = &data->script;
	env.backend   = backend;
	
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

backend_t rewrite_proto = {
	.class          = "request/rewrite",
	.supported_api  = API_HASH,
	.func_init      = &rewrite_init,
	.func_configure = &rewrite_configure,
	.func_destroy   = &rewrite_destroy,
	.backend_type_hash = {
		.func_handler = &rewrite_func,
	}
};

