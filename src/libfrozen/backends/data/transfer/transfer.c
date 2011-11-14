#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_transfer Backend 'data/transfer'
 */
/**
 * @ingroup mod_backend_transfer
 * @page page_transfer_info Description
 *
 * This module actively transfer data from one backend to another
 */
/**
 * @ingroup mod_backend_transfer
 * @page page_transfer_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/transfer",
 *              source                  = (backend_t)'',      # source backend, default - underlying backend
 *              destination             = (backend_t)''       # destination backend, default - underlying backend
 * }
 * @endcode
 */

#define EMODULE 39

typedef struct transfer_userdata {
	backend_t             *source;
	backend_t             *destination;
	uintmax_t              running;
	pthread_t              tranfser_thread;
} transfer_userdata;

static void * tranfser_thread_handler(backend_t *backend){ // {{{
	backend_t             *source;
	backend_t             *destination;
	transfer_userdata     *userdata          = (transfer_userdata *)backend->userdata;
	
	source      = userdata->source      ? userdata->source      : backend; // backend have no handlers, so it will pass to underlying
	destination = userdata->destination ? userdata->destination : backend;
	
	data_t  d_source      = DATA_BACKENDT(source);
	data_t  d_destination = DATA_BACKENDT(destination);
	
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_destination };
	data_query(&d_source, &r_transfer);
	
	// TODO on_end motion
	return NULL;
} // }}}

static int transfer_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(transfer_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int transfer_destroy(backend_t *backend){ // {{{
	ssize_t                   ret;
	void                     *res;
	transfer_userdata        *userdata          = (transfer_userdata *)backend->userdata;
	
	if(userdata->running != 0){
		switch(pthread_cancel(userdata->tranfser_thread)){
			case ESRCH: goto no_thread;
			case 0:     break;
			default:    ret = error("pthread_cancel failed"); goto no_thread;
		}
		if(pthread_join(userdata->tranfser_thread, &res) != 0)
			ret = error("pthread_join failed");
		
		no_thread:
		userdata->running = 0;
	}
	if(userdata->source)
		backend_destroy(userdata->source);
	if(userdata->destination)
		backend_destroy(userdata->destination);
	
	free(userdata);
	return ret;
} // }}}
static int transfer_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	transfer_userdata     *userdata          = (transfer_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_BACKENDT,   userdata->source,        config, HK(source));
	hash_data_copy(ret, TYPE_BACKENDT,   userdata->destination,   config, HK(destination));
	
	if(userdata->running == 0){
		if(pthread_create(&userdata->tranfser_thread, NULL, (void *(*)(void *)) &tranfser_thread_handler, backend) != 0)
			ret = error("pthread_create failed");
		
		userdata->running = 1;
	}
	return 0;
} // }}}

backend_t transfer_proto = {
	.class          = "data/transfer",
	.supported_api  = API_HASH,
	.func_init      = &transfer_init,
	.func_configure = &transfer_configure,
	.func_destroy   = &transfer_destroy,
};

