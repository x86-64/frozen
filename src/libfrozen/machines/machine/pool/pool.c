#include <libfrozen.h>
#include <shop/pass/pass.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_pool machine/pool
 *
 * This module keep pool of given pipelines_pool and manage them.
 */
/**
 * @ingroup mod_machine_pool
 * @page page_pool_config Configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class             = "machine/pool",
 * 	        config            = { ... },               // hash with shop configuration
 * 	        size              = (uint_t)"10"           // initial pool size, default 1
 * 	}
 * @endcode
 */

#define ERRORS_MODULE_ID 14
#define ERRORS_MODULE_NAME "machine/pool"

typedef struct pool_userdata {
	list                   pipelines_pool;
	request_t             *prototype;
} pool_userdata;

static ssize_t pool_resize(machine_t *pool, uintmax_t new_size){ // {{{
	ssize_t                ret;
	hash_t                *pipelines;
	uintmax_t              current_size;
	request_t             *prototype;
	pool_userdata         *userdata          = (pool_userdata *)pool->userdata;
	
	current_size = list_count(&userdata->pipelines_pool);
	
	for(; current_size < new_size; current_size++){
		prototype = hash_copy(userdata->prototype);
		
		if( (ret = pipelines_new(&pipelines, prototype)) < 0)
			return ret;
		
		hash_free(prototype);
		if(ret < 0)
			return ret;
		
		list_add(&userdata->pipelines_pool, pipelines);
	}
	for(; current_size > new_size; current_size--){
		pipelines = list_pop(&userdata->pipelines_pool);
		
		hash_free(pipelines);
	}
	return 0;
} // }}}

static ssize_t pool_init(machine_t *machine){ // {{{
	pool_userdata         *userdata          = machine->userdata = calloc(1, sizeof(pool_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	list_init(&userdata->pipelines_pool);
	return 0;
} // }}}
static ssize_t pool_destroy(machine_t *machine){ // {{{
	pool_userdata         *userdata          = (pool_userdata *)machine->userdata;
	
	pool_resize(machine, 0);
	list_destroy(&userdata->pipelines_pool);
	return 0;
} // }}}
static ssize_t pool_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              initial_size      = 1;
	pool_userdata         *userdata          = (pool_userdata *)machine->userdata;
	
	hash_data_get    (ret, TYPE_UINTT, initial_size,        config, HK(size));
	hash_data_consume(ret, TYPE_HASHT, userdata->prototype, config, HK(config));
	if(ret != 0)
		return error("config not supplied");
	
	pool_resize(machine, initial_size);
	return 0;
} // }}}

static ssize_t pool_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	action_t               action;
	hashkey_t              function;
	hash_t                *pipeline;
	void                  *iter              = NULL;
	pool_userdata         *userdata          = (pool_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_ACTIONT, action, request, HK(action));
	if(ret != 0 || action != ACTION_CONTROL)
		return errorn(ENOSYS);
	
	hash_data_get(ret, TYPE_HASHKEYT, function, request, HK(function));
	if(ret != 0)
		return errorn(ENOSYS);
	
	switch(function){
		case HK(start):
			while( (pipeline = list_iter_next(&userdata->pipelines_pool, &iter)) != NULL){
				if( (ret = pipelines_execute(pipeline)) < 0)
					return ret;
			}
			return 0;
			
		default:
			break;
	}
	return errorn(ENOSYS);
} // }}}

machine_t pool_proto = {
	.class          = "machine/pool",
	.supported_api  = API_HASH,
	.func_init      = &pool_init,
	.func_configure = &pool_configure,
	.func_destroy   = &pool_destroy,
	.machine_type_hash = {
		.func_handler = &pool_handler
	}
};

