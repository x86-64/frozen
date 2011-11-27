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
 *         class        = "daemon/thread",
 *         paused       = (uint_t)'0'                # do not start thread after configure
 * }
 * @threadcode
 */

#define EMODULE 41

typedef struct thread_userdata {
	//api_t                apitype; // TODO api types
	uintmax_t              paused;
	
	uintmax_t              thread_running;
	pthread_t              thread;
} thread_userdata;

static void *  thread_routine(backend_t *backend){ // {{{
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
} // }}}
static ssize_t thread_control_start(backend_t *backend){ // {{{
	thread_userdata       *userdata          = (thread_userdata *)backend->userdata;
	
	if(userdata->thread_running == 0){
		if(pthread_create(&userdata->thread, NULL, (void * (*)(void *))&thread_routine, backend) != 0)
			return error("pthread_create failed");
		
		userdata->thread_running = 1;
	}
	return 0;
} // }}}
static ssize_t thread_control_stop(backend_t *backend){ // {{{
	ssize_t                ret;
	void                  *res;
	thread_userdata       *userdata          = (thread_userdata *)backend->userdata;
	
	if(userdata->thread_running != 0){
		switch(pthread_cancel(userdata->thread)){
			case ESRCH: goto no_thread;
			case 0:     break;
			default:    ret = error("pthread_cancel failed"); goto no_thread;
		}
		if(pthread_join(userdata->thread, &res) != 0)
			ret = error("pthread_join failed");
		
		no_thread:
		userdata->thread_running = 0;
	}
	return ret;
} // }}}

static int thread_init(backend_t *backend){ // {{{
	thread_userdata         *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(thread_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int thread_destroy(backend_t *backend){ // {{{
	ssize_t                ret;
	thread_userdata       *userdata          = (thread_userdata *)backend->userdata;
	
	ret = thread_control_stop(backend);
	
	free(userdata);
	return ret;
} // }}}
static int thread_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	thread_userdata       *userdata          = (thread_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_UINTT, userdata->paused, config, HK(paused));
	
	if(userdata->paused == 0)
		thread_control_start(backend);
	
	return 0;
} // }}}

static ssize_t thread_fast_handler(backend_t *backend, fastcall_header *hargs){ // {{{
	switch(hargs->action){
		case ACTION_START: return thread_control_start(backend);
		case ACTION_STOP:  return thread_control_stop(backend);
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t thread_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              action;
	
	hash_data_copy(ret, TYPE_UINTT, action, request, HK(action));
	
	fastcall_header r_fast = { 2, action };
	return thread_fast_handler(backend, &r_fast);
} // }}}

backend_t thread_proto = {
	.class          = "daemon/thread",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = thread_init,
	.func_destroy   = thread_destroy,
	.func_configure = thread_configure,
	.backend_type_hash = {
		.func_handler = &thread_handler
	},
	.backend_type_fast = {
		.func_handler = (f_fast_func)&thread_fast_handler
	}
};

