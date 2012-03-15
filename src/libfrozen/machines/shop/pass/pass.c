#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_pass shop/pass
 */
/**
 * @ingroup mod_machine_pass
 * @page page_pass_info Description
 *
 * This machine pass current request to user-supplied machine.
 */
/**
 * @ingroup mod_machine_pass
 * @page page_pass_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "shop/pass",
 *              shop                    =                         // pass to this shop
 *                                        (env_t)'machine',       //  - to shop supplied in user request under "machine" hashkey 
 *                                        (machine_t)'name',      //  - to shop named "name"
 *                                        ...
 * }
 * @endcode
 */
/**
 * @ingroup machine
 * @addtogroup mod_machine_return shop/return
 */
/**
 * @ingroup mod_machine_return
 * @page page_return_info Description
 *
 * This machine return current request to user-supplied machine.
 */
/**
 * @ingroup mod_machine_return
 * @page page_return_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "shop/return"
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID 25
#define ERRORS_MODULE_NAME "shop/pass"

pthread_key_t                  key_stack;
pthread_once_t                 key_stack_once    = PTHREAD_ONCE_INIT;

static void        stack_destructor(void *stack){ // {{{
	list_free(stack);
} // }}}
static void        stack_init(void){ // {{{
	pthread_key_create(&key_stack, &stack_destructor);
} // }}}
static list *      stack_get_stack(void){ // {{{
	list                  *stack;
	
	pthread_once(&key_stack_once, stack_init);
	if( (stack = pthread_getspecific(key_stack)) == NULL){
		stack = list_alloc();
		pthread_setspecific(key_stack, stack);
	}
	return stack;
} // }}}

ssize_t            stack_call(machine_t *return_to){ // {{{
	list                  *stack;
	
	if( (stack = stack_get_stack()) == NULL)
		return -EINVAL;
	
	list_push(stack, return_to);
	return 0;
} // }}}
ssize_t            stack_clean(void){ // {{{
	list                  *stack;
	
	if( (stack = stack_get_stack()) == NULL)
		return -EINVAL;
	
	list_pop(stack);
	return 0;
} // }}}
machine_t *        stack_return(void){ // {{{
	machine_t             *curr;
	list                  *stack;
	void                  *iter              = NULL;
	
	if( (stack = stack_get_stack()) == NULL)
		return NULL;
	
	while( (curr = list_iter_next(stack, &iter), iter != NULL) ){
		if(curr != NULL){
			list_delete(stack, curr);
			return curr;
		}
	}
	return NULL;
} // }}}


typedef struct pass_userdata {
	data_t                 machine;
} pass_userdata;

static ssize_t pass_init(machine_t *machine){ // {{{
	pass_userdata         *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(pass_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static ssize_t pass_destroy(machine_t *machine){ // {{{
	pass_userdata      *userdata = (pass_userdata *)machine->userdata;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&userdata->machine, &r_free);
	
	free(userdata);
	return 0;
} // }}}
static ssize_t pass_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	pass_userdata         *userdata          = (pass_userdata *)machine->userdata;
	
	hash_holder_consume(ret, userdata->machine, config, HK(shop));
	if(ret != 0)
		return error("shop parameter not supplied");
	
	return 0;
} // }}}

static ssize_t pass_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	pass_userdata         *userdata          = (pass_userdata *)machine->userdata;
	
	request_enter_context(request);
	
		stack_call(machine->cnext);
		
			fastcall_query r_query = { { 3, ACTION_QUERY }, request };
			ret = data_query(&userdata->machine, &r_query);
			
		stack_clean();

	request_leave_context();
	return ret;
} // }}}

machine_t pass_proto = {
	.class          = "shop/pass",
	.supported_api  = API_HASH,
	.func_init      = &pass_init,
	.func_destroy   = &pass_destroy,
	.func_configure = &pass_configure,
	.machine_type_hash = {
		.func_handler = &pass_handler
	}
};

static ssize_t return_handler(machine_t *machine, request_t *request){ // {{{
	machine_t               *return_to       = stack_return();
	
	if(return_to == NULL)
		return error("no return address supplied");
	
	return machine_query(return_to, request);
} // }}}

machine_t return_proto = {
	.class          = "shop/return",
	.supported_api  = API_HASH,
	.machine_type_hash = {
		.func_handler = &return_handler
	}
};

