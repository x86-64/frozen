#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_wrapper backend/wrapper
 */
/**
 * @ingroup mod_backend_wrapper
 * @page page_wrapper_info Description
 *
 * This backend wrap any backend chain on top of some data. This can be used to create complex data fields in structures.
 * 
 * Backend chain must talk with last backend only by API_FAST calls, because data can only parse those.
 */
/**
 * @ingroup mod_backend_wrapper
 * @page page_wrapper_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "backend/wrapper",
 *              config                  = { ... },            # backend configuration
 *              data                    = (hashkey_t)'0',     # on which data wrap backend, default "input"
 * }
 * @endcode
 */

#define EMODULE 48

typedef struct wrapper_userdata {
	thread_data_ctx_t      thread_data;
	
	backend_t             *backend;
	hashkey_t              data_hk;
} wrapper_userdata;

typedef struct wrapper_threaddata {
	data_t                *data;
} wrapper_threaddata;

static ssize_t wrapper_terminator_fast_handler(backend_t *backend, void *hargs){ // {{{
	wrapper_userdata      *userdata          = (wrapper_userdata *)backend->userdata;
	wrapper_threaddata    *threaddata        = thread_data_get(&userdata->thread_data);
	
	return data_query(threaddata->data, hargs);
} // }}}

static void * wrapper_threaddata_create(void *userdata){ // {{{
	return malloc(sizeof(wrapper_threaddata));
} // }}}
static int wrapper_init(backend_t *backend){ // {{{
	wrapper_userdata      *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(wrapper_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->backend = NULL;
	userdata->data_hk = HK(input);
	
	return thread_data_init(
		&userdata->thread_data, 
		&wrapper_threaddata_create,
		&free,
		NULL);
} // }}}
static int wrapper_destroy(backend_t *backend){ // {{{
	wrapper_userdata      *userdata          = (wrapper_userdata *)backend->userdata;
	
	if(userdata->backend)
		backend_destroy(userdata->backend);
	
	free(userdata);
	return 0;
} // }}}
static int wrapper_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	backend_t             *terminator;
	list                   term_list         = LIST_INITIALIZER;
	wrapper_userdata      *userdata          = (wrapper_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT,   userdata->data_hk, config, HK(data));
	
	if(userdata->backend == NULL){
		hash_data_copy(ret, TYPE_BACKENDT, userdata->backend, config, HK(config));
		if(ret != 0)
			return error("backend config not supplied or invalid");
			
		if( (terminator = backend_clone(backend)) == NULL){
			backend_destroy(userdata->backend);
			userdata->backend = NULL;
			return error("can not create terminator backend");
		}
		
		terminator->func_configure                 = NULL;
		terminator->func_destroy                   = NULL;
		terminator->supported_api                  = API_FAST;
		terminator->backend_type_fast.func_handler = &wrapper_terminator_fast_handler;
		
		list_add(&term_list, terminator);
		backend_add_terminators(userdata->backend, &term_list);
	}
	return 0;
} // }}}

static ssize_t wrapper_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	wrapper_userdata      *userdata          = (wrapper_userdata *)backend->userdata;
	wrapper_threaddata    *threaddata        = thread_data_get(&userdata->thread_data);
	
	if( (threaddata->data = hash_data_find(request, userdata->data_hk)) == NULL)
		return error("no data in request");
	
	if( (ret = backend_query(userdata->backend, request)) < 0)
		return ret;
	
	return ( (ret = backend_pass(backend, request)) < 0 ) ? ret : -EEXIST;
} // }}}

backend_t wrapper_proto = {
	.class          = "backend/wrapper",
	.supported_api  = API_HASH,
	.func_init      = &wrapper_init,
	.func_destroy   = &wrapper_destroy,
	.func_configure = &wrapper_configure,
	.backend_type_hash = {
		.func_handler = &wrapper_handler
	}
};

