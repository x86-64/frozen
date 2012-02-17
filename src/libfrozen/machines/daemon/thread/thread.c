#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_thread daemon/thread
 */
/**
 * @ingroup mod_machine_thread
 * @page page_thread_info Description
 *
 * This machine create new thread which emit empty request to underlying machine.
 */
/**
 * @ingroup mod_machine_thread
 * @page page_thread_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *         class            = "daemon/thread",
 *         paused           = (uint_t)'0',               # do not start thread after configure
 *         loop             = (uint_t)'1'                # run thread in infinity loop
 *         ignore_errors    = (uint_t)'1'                # ignore errors and do not exit from loop, default 0
 *         destroy          = (uint_t)'1'                # destroy machine shop on exit from loop, default 1
 * }
 * @threadcode
 */

#define EMODULE 41

typedef struct thread_userdata {
	uintmax_t              paused;
	uintmax_t              loop;
	uintmax_t              ignore_errors;
	uintmax_t              destroy_on_exit;
	
	uintmax_t              self_termination;
	uintmax_t              thread_running;
	pthread_t              thread;
	pthread_mutex_t        thread_mutex;
} thread_userdata;

static void *  thread_routine(machine_t *machine){ // {{{
	ssize_t                ret;
	thread_userdata       *userdata          = (thread_userdata *)machine->userdata;
	
	sleep(0); // BUG improper fix for too fast thread creation, before machine connected to chain
	do{
		request_t r_request[] = { hash_end };
		if( (ret = machine_pass(machine, r_request)) < 0){
			log_error("thread error: %d: %s\n", ret, describe_error(ret));
			
			if(userdata->ignore_errors == 0)
				break;
		}
	}while(userdata->loop);
	
	pthread_mutex_lock(&destroy_mtx); // TODO remove this..
	pthread_mutex_lock(&userdata->thread_mutex);
		userdata->thread_running = 0;
		
		if(userdata->destroy_on_exit){
			userdata->self_termination = 1;
			shop_destroy(machine);
			goto self_free;
		}
	pthread_mutex_unlock(&userdata->thread_mutex);
	pthread_mutex_unlock(&destroy_mtx); // TODO remove this..
	return NULL;

self_free:
	pthread_mutex_unlock(&userdata->thread_mutex);
	pthread_mutex_unlock(&destroy_mtx);
	pthread_mutex_destroy(&userdata->thread_mutex);
	free(userdata);
	return NULL;
} // }}}
static ssize_t thread_control_start(machine_t *machine){ // {{{
	ssize_t                ret               = 0;
	pthread_attr_t         attr;
	thread_userdata       *userdata          = (thread_userdata *)machine->userdata;
	
	pthread_mutex_lock(&userdata->thread_mutex);
		if(userdata->thread_running == 0){
			userdata->thread_running = 1;
			userdata->self_termination = 0;

			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
			
			if(pthread_create(&userdata->thread, &attr, (void * (*)(void *))&thread_routine, machine) != 0)
				ret = error("pthread_create failed");
			
			pthread_attr_destroy(&attr);
		}
	pthread_mutex_unlock(&userdata->thread_mutex);
	return ret;
} // }}}
static ssize_t thread_control_stop(machine_t *machine){ // {{{
	thread_userdata       *userdata          = (thread_userdata *)machine->userdata;
	
	pthread_mutex_lock(&userdata->thread_mutex);
		if(userdata->thread_running == 1){
			userdata->thread_running = 0;
			pthread_cancel(userdata->thread);
		}
	pthread_mutex_unlock(&userdata->thread_mutex);
	return 0;
} // }}}

static int thread_init(machine_t *machine){ // {{{
	pthread_mutexattr_t    attr;
	thread_userdata       *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(thread_userdata))) == NULL)
		return error("calloc failed");
	
	if(pthread_mutexattr_init(&attr) != 0)
		return -EFAULT;
		
	if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
		return -EFAULT;
	
	if(pthread_mutex_init(&userdata->thread_mutex, &attr) != 0)
		return -EFAULT;
	
	pthread_mutexattr_destroy(&attr);

	userdata->destroy_on_exit = 1;
	return 0;
} // }}}
static int thread_destroy(machine_t *machine){ // {{{
	thread_userdata       *userdata          = (thread_userdata *)machine->userdata;
	
	if(userdata->self_termination == 1)
		return 0;
	
	thread_control_stop(machine);
	
	pthread_mutex_destroy(&userdata->thread_mutex);
	free(userdata);
	return 0;
} // }}}
static int thread_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	thread_userdata       *userdata          = (thread_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_UINTT, userdata->paused,            config, HK(paused));
	hash_data_get(ret, TYPE_UINTT, userdata->loop,              config, HK(loop));
	hash_data_get(ret, TYPE_UINTT, userdata->ignore_errors,     config, HK(ignore_errors));
	hash_data_get(ret, TYPE_UINTT, userdata->destroy_on_exit,   config, HK(destroy));
	
	if(userdata->paused == 0)
		thread_control_start(machine);
	
	return 0;
} // }}}

static ssize_t thread_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	action_t               action;
	
	hash_data_get(ret, TYPE_ACTIONT, action, request, HK(action));
	
	switch(action){
		case ACTION_START: return thread_control_start(machine);
		case ACTION_STOP:  return thread_control_stop(machine);
		default:
			break;
	}
	return -ENOSYS;
} // }}}

machine_t thread_proto = {
	.class          = "daemon/thread",
	.supported_api  = API_HASH,
	.func_init      = thread_init,
	.func_destroy   = thread_destroy,
	.func_configure = thread_configure,
	.machine_type_hash = {
		.func_handler = &thread_handler
	},
};

