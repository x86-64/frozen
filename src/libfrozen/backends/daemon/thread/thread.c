#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_thread daemon/thread
 */
/**
 * @ingroup mod_backend_thread
 * @page page_thread_info Description
 *
 * This backend create new thread which emit empty request to underlying backend.
 */
/**
 * @ingroup mod_backend_thread
 * @page page_thread_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class = "daemon/thread"
 * }
 * @threadcode
 */

#define EMODULE 41

typedef struct thread_userdata {
	//api_t                apitype; // TODO api types
	pthread_t              thread;
	uintmax_t              started;
} thread_userdata;

static void * thread_handler(backend_t *backend){
	//switch(apitype){
	//     case API_HASH:;
			request_t r_request[] = {
				hash_end
			};
			backend_pass(backend, r_request);
	//		break;
	//     case API_FAST:;
	//}
	return NULL;
}

static int thread_init(backend_t *backend){ // {{{
	thread_userdata         *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(thread_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int thread_destroy(backend_t *backend){ // {{{
	ssize_t                ret      = 0;
	void                  *res;
	thread_userdata       *userdata = (thread_userdata *)backend->userdata;
	
	if(userdata->started != 0){
		switch(pthread_cancel(userdata->thread)){
			case ESRCH: goto no_thread;
			case 0:     break;
			default:    ret = error("pthread_cancel failed"); goto no_thread;
		}
		if(pthread_join(userdata->thread, &res) != 0)
			ret = error("pthread_join failed");
		
		no_thread:
		userdata->started = 0;
	}
	
	free(userdata);
	return ret;
} // }}}
static int thread_configure(backend_t *backend, config_t *config){ // {{{
	thread_userdata       *userdata          = (thread_userdata *)backend->userdata;
	
	if(userdata->started == 0){
		if(pthread_create(&userdata->thread, NULL, (void * (*)(void *))&thread_handler, backend) != 0)
			return error("pthread_create failed");
		
		userdata->started = 1;
	}
	return 0;
} // }}}

backend_t thread_proto = {
	.class          = "daemon/thread",
	.supported_api  = API_HASH,
	.func_init      = thread_init,
	.func_destroy   = thread_destroy,
	.func_configure = thread_configure,
};

