#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_wrapper machine/wrapper
 */
/**
 * @ingroup mod_machine_wrapper
 * @page page_wrapper_info Description
 *
 * This machine wrap any machine shop on top of some data. This can be used to create complex data fields in structures.
 * 
 * Machine shop must talk with last machine only by API_FAST calls, because data can only parse those.
 */
/**
 * @ingroup mod_machine_wrapper
 * @page page_wrapper_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "machine/wrapper",
 *              config                  = { ... },            # machine configuration
 *              data                    = (hashkey_t)'0',     # on which data wrap machine, default "input"
 * }
 * @endcode
 */

#define EMODULE 48

typedef struct wrapper_userdata {
	thread_data_ctx_t      thread_data;
	
	machine_t             *machine;
	hashkey_t              data_hk;
} wrapper_userdata;

typedef struct wrapper_threaddata {
	data_t                *data;
} wrapper_threaddata;

static ssize_t wrapper_terminator_fast_handler(machine_t *machine, void *hargs){ // {{{
	wrapper_userdata      *userdata          = (wrapper_userdata *)machine->userdata;
	wrapper_threaddata    *threaddata        = thread_data_get(&userdata->thread_data);
	
	return data_query(threaddata->data, hargs);
} // }}}

static void * wrapper_threaddata_create(void *userdata){ // {{{
	return malloc(sizeof(wrapper_threaddata));
} // }}}
static int wrapper_init(machine_t *machine){ // {{{
	wrapper_userdata      *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(wrapper_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->machine = NULL;
	userdata->data_hk = HK(input);
	
	return thread_data_init(
		&userdata->thread_data, 
		&wrapper_threaddata_create,
		&free,
		NULL);
} // }}}
static int wrapper_destroy(machine_t *machine){ // {{{
	wrapper_userdata      *userdata          = (wrapper_userdata *)machine->userdata;
	
	if(userdata->machine)
		machine_destroy(userdata->machine);
	
	free(userdata);
	return 0;
} // }}}
static int wrapper_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	machine_t             *terminator;
	list                   term_list         = LIST_INITIALIZER;
	wrapper_userdata      *userdata          = (wrapper_userdata *)machine->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT,   userdata->data_hk, config, HK(data));
	
	if(userdata->machine == NULL){
		hash_data_copy(ret, TYPE_MACHINET, userdata->machine, config, HK(config));
		if(ret != 0)
			return error("machine config not supplied or invalid");
			
		if( (terminator = machine_clone(machine)) == NULL){
			machine_destroy(userdata->machine);
			userdata->machine = NULL;
			return error("can not create terminator machine");
		}
		
		terminator->func_configure                 = NULL;
		terminator->func_destroy                   = NULL;
		terminator->supported_api                  = API_FAST;
		terminator->machine_type_fast.func_handler = &wrapper_terminator_fast_handler;
		
		list_add(&term_list, terminator);
		//machine_add_terminators(userdata->machine, &term_list);
	}
	return 0;
} // }}}

static ssize_t wrapper_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	wrapper_userdata      *userdata          = (wrapper_userdata *)machine->userdata;
	wrapper_threaddata    *threaddata        = thread_data_get(&userdata->thread_data);
	
	if( (threaddata->data = hash_data_find(request, userdata->data_hk)) == NULL)
		return error("no data in request");
	
	if( (ret = machine_query(userdata->machine, request)) < 0)
		return ret;
	
	return ( (ret = machine_pass(machine, request)) < 0 ) ? ret : -EEXIST;
} // }}}

machine_t wrapper_proto = {
	.class          = "machine/wrapper",
	.supported_api  = API_HASH,
	.func_init      = &wrapper_init,
	.func_destroy   = &wrapper_destroy,
	.func_configure = &wrapper_configure,
	.machine_type_hash = {
		.func_handler = &wrapper_handler
	}
};

