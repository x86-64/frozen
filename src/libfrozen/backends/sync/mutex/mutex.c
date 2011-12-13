#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_mutex sync/mutex
 */
/**
 * @ingroup mod_backend_mutex
 * @page page_mutex_info Description
 *
 * This backend lock mutex on incoming request and unlock then request ends. It can be used to ensure that only one
 * thread would be in underlying backend.
 */
/**
 * @ingroup mod_backend_mutex
 * @page page_mutex_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "sync/mutex"
 * }
 * @endcode
 */

#define EMODULE 44

typedef struct mutex_userdata {
	pthread_mutex_t        mutex;
} mutex_userdata;

static int mutex_init(backend_t *backend){ // {{{
	pthread_mutexattr_t    attr;
	mutex_userdata        *userdata          = backend->userdata = calloc(1, sizeof(mutex_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	if(pthread_mutexattr_init(&attr) != 0)
		return error("pthread_mutexattr_init failed");
		
	if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		return error("pthread_mutexattr_settype failed");
	
	if(pthread_mutex_init(&userdata->mutex, &attr) != 0)
		return error("pthread_mutex_init failed");
	
	pthread_mutexattr_destroy(&attr);
	return 0;
} // }}}
static int mutex_destroy(backend_t *backend){ // {{{
	mutex_userdata          *userdata          = (mutex_userdata *)backend->userdata;
	
	pthread_mutex_destroy(&userdata->mutex);
	free(userdata);
	return 0;
} // }}}

static ssize_t mutex_request(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	mutex_userdata        *userdata          = (mutex_userdata *)backend->userdata;
	
	pthread_mutex_lock(&userdata->mutex);
	
		ret = ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	pthread_mutex_unlock(&userdata->mutex);
	return ret;
} // }}}
static ssize_t mutex_fast_request(backend_t *backend, void *hargs){ // {{{
	ssize_t                ret;
	mutex_userdata        *userdata          = (mutex_userdata *)backend->userdata;
	
	pthread_mutex_lock(&userdata->mutex);
		
		ret = ( (ret = backend_fast_pass(backend, hargs)) < 0) ? ret : -EEXIST;
	
	pthread_mutex_unlock(&userdata->mutex);
	return ret;
} // }}}

backend_t mutex_proto = {
	.class          = "sync/mutex",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = &mutex_init,
	.func_destroy   = &mutex_destroy,
	.backend_type_hash = {
		.func_handler = &mutex_request
	},
	.backend_type_fast = {
		.func_handler = &mutex_fast_request
	}
};
