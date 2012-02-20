#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_mutex sync/mutex
 */
/**
 * @ingroup mod_machine_mutex
 * @page page_mutex_info Description
 *
 * This machine lock mutex on incoming request and unlock then request ends. It can be used to ensure that only one
 * thread would be in underlying machine.
 */
/**
 * @ingroup mod_machine_mutex
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

static ssize_t mutex_init(machine_t *machine){ // {{{
	pthread_mutexattr_t    attr;
	mutex_userdata        *userdata          = machine->userdata = calloc(1, sizeof(mutex_userdata));
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
static ssize_t mutex_destroy(machine_t *machine){ // {{{
	mutex_userdata          *userdata          = (mutex_userdata *)machine->userdata;
	
	pthread_mutex_destroy(&userdata->mutex);
	free(userdata);
	return 0;
} // }}}

static ssize_t mutex_request(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	mutex_userdata        *userdata          = (mutex_userdata *)machine->userdata;
	
	pthread_mutex_lock(&userdata->mutex);
	
		ret = machine_pass(machine, request);
	
	pthread_mutex_unlock(&userdata->mutex);
	return ret;
} // }}}

machine_t mutex_proto = {
	.class          = "sync/mutex",
	.supported_api  = API_HASH,
	.func_init      = &mutex_init,
	.func_destroy   = &mutex_destroy,
	.machine_type_hash = {
		.func_handler = &mutex_request
	},
};

