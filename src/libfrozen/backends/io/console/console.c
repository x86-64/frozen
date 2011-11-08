#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_stdin Backend 'io/stdin'
 */
/**
 * @ingroup mod_backend_stdin
 * @page page_stdin_info Description
 *
 * This backend read from console
 */
/**
 * @ingroup mod_backend_stdin
 * @page page_stdin_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "io/stdin",
 *              action                  = (action_t)'write',       # action to emit, default "write"
 *              output                  = (hashkey_t)'buffer',     # output buffer key, default "buffer"
 *              destroy                 = (uint_t)'1'              # send SEGTERM on stdin eof, default 0
 * }
 * @endcode
 */

/**
 * @ingroup backend
 * @addtogroup mod_backend_stdout Backend 'io/stdout'
 */
/**
 * @ingroup mod_backend_stdout
 * @page page_stdout_info Description
 *
 * This backend write to console stdout
 */
/**
 * @ingroup mod_backend_stdout
 * @page page_stdout_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "io/stdout",
 *              input                   = (hashkey_t)'buffer'     # input buffer key, default "buffer"
 * }
 * @endcode
 */

/**
 * @ingroup backend
 * @addtogroup mod_backend_stderr Backend 'io/stderr'
 */
/**
 * @ingroup mod_backend_stderr
 * @page page_stderr_info Description
 *
 * This backend write to console stderr
 */
/**
 * @ingroup mod_backend_stderr
 * @page page_stderr_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "io/stderr",
 *              input                   = (hashkey_t)'buffer'     # input buffer key, default "buffer"
 * }
 * @endcode
 */

#define EMODULE 38

typedef struct stdin_userdata {
	data_functions         action;
	hash_key_t             output;
	uintmax_t              running;
	uintmax_t              kill_on_eof;
} stdin_userdata;

typedef struct stdout_userdata {
	hash_key_t             input;
} stdout_userdata;

static pthread_t               stdin_thread;
static uintmax_t               stdin_backends    = 0;
static pthread_mutex_t         stdin_backend_mtx = PTHREAD_MUTEX_INITIALIZER;
static list                    stdin_childs;

static ssize_t stdout_io_handler(FILE **fd, fastcall_header *hargs){ // {{{
	switch(hargs->action){
		case ACTION_WRITE:;
			fastcall_write *wargs = (fastcall_write *)hargs;
			
			wargs->buffer_size = fwrite(wargs->buffer, 1, wargs->buffer_size, *fd);
			return 0;

		default:
			break;
	}
	return -ENOSYS;
} // }}}

static data_t stdout_io = DATA_IOT(&stdout, (f_io_func)&stdout_io_handler);
static data_t stderr_io = DATA_IOT(&stderr, (f_io_func)&stdout_io_handler);

static void * stdin_thread_handler(void *null){ // {{{
	uintmax_t              ret;
	backend_t             *child;
	void                  *iter;
	char                   buffer[DEF_BUFFER_SIZE];
	
	while(!feof(stdin)){
		ret = fread(buffer, 1, sizeof(buffer), stdin);
		if(ret == 0)
			goto exit;
		
		list_rdlock(&stdin_childs);
			iter = NULL;
			
			while( (child = list_iter_next(&stdin_childs, &iter)) != NULL){
				stdin_userdata *userdata = (stdin_userdata *)child->userdata;
				
				request_t request[] = {
					{ HK(action),       DATA_UINT32T(userdata->action)  },
					{ userdata->output, DATA_RAW(buffer, ret)           },
					hash_end
				};
				backend_pass(child, request);
			}
		list_unlock(&stdin_childs);
	}
exit:
	
	list_rdlock(&stdin_childs);
		iter = NULL;
		
		while( (child = list_iter_next(&stdin_childs, &iter)) != NULL){
			stdin_userdata *userdata = (stdin_userdata *)child->userdata;
			
			if(userdata->kill_on_eof != 0)
				kill(getpid(), SIGTERM);
		}
	list_unlock(&stdin_childs);
	return NULL;
} // }}}

static int stdin_init(backend_t *backend){ // {{{
	stdin_userdata        *userdata;

	if((userdata = backend->userdata = calloc(1, sizeof(stdin_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->action       = ACTION_WRITE;
	userdata->output       = HK(buffer);
	userdata->kill_on_eof  = 0;
	return 0;
} // }}}
static int stdin_destroy(backend_t *backend){ // {{{
	void                  *res;
	ssize_t                ret               = 0;
	stdin_userdata        *userdata          = (stdin_userdata *)backend->userdata;

	list_delete (&stdin_childs, backend);
	pthread_mutex_lock(&stdin_backend_mtx);
		if(--stdin_backends == 0){
			switch(pthread_cancel(stdin_thread)){
				case ESRCH: goto no_thread;
				case 0:     break;
				default:    ret = error("pthread_cancel failed"); goto exit;
			}
			if(pthread_join(stdin_thread, &res) != 0){
				ret = error("pthread_join failed");
				goto exit;
			}
			
		no_thread:
			list_destroy(&stdin_childs);
		}
exit:
	pthread_mutex_unlock(&stdin_backend_mtx);
	
	free(userdata);
	return ret;
} // }}}
static int stdin_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	char                  *action_str        = NULL;
	stdin_userdata        *userdata          = (stdin_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_UINTT,    userdata->kill_on_eof, config, HK(destroy));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->output,      config, HK(output));
	hash_data_copy(ret, TYPE_STRINGT,  action_str,            config, HK(action));
	if(ret == 0)
		userdata->action = request_str_to_action(action_str);
	
	ret = 0;
	if(userdata->running == 0){
		pthread_mutex_lock(&stdin_backend_mtx);
			if(stdin_backends++ == 0){
				list_init(&stdin_childs);
				
				if(pthread_create(&stdin_thread, NULL, &stdin_thread_handler, NULL) != 0)
					ret = error("pthread_create failed");
			}
		pthread_mutex_unlock(&stdin_backend_mtx);
		list_add(&stdin_childs, backend);
		
		userdata->running = 1;
	}
	return ret;
} // }}}

static int stdout_init(backend_t *backend){ // {{{
	stdout_userdata       *userdata;

	if((userdata = backend->userdata = calloc(1, sizeof(stdout_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->input       = HK(buffer);
	return 0;
} // }}}
static int stdout_destroy(backend_t *backend){ // {{{
	free(backend->userdata);
	return 0;
} // }}}
static int stdout_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	stdout_userdata       *userdata          = (stdout_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->input,      config, HK(input));
	return 0;
} // }}}

static ssize_t stdout_handler(backend_t *backend, request_t *request){ // {{{
	data_t                *input;
	stdout_userdata       *userdata          = (stdout_userdata *)backend->userdata;
	
	if( (input = hash_data_find(request, userdata->input)) == NULL)
		return error("input key not supplied");
	
	static fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &stdout_io };
	return data_query(input, &r_transfer);
} // }}}
static ssize_t stderr_handler(backend_t *backend, request_t *request){ // {{{
	data_t                *input;
	stdout_userdata       *userdata          = (stdout_userdata *)backend->userdata;
	
	if( (input = hash_data_find(request, userdata->input)) == NULL)
		return error("input key not supplied");
	
	static fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &stderr_io };
	return data_query(input, &r_transfer);
} // }}}

backend_t stdin_proto = {
	.class          = "io/stdin",
	.supported_api  = API_HASH,
	.func_init      = &stdin_init,
	.func_configure = &stdin_configure,
	.func_destroy   = &stdin_destroy
};

backend_t stdout_proto = {
	.class          = "io/stdout",
	.supported_api  = API_HASH,
	.func_init      = &stdout_init,
	.func_configure = &stdout_configure,
	.func_destroy   = &stdout_destroy,
	.backend_type_hash = {
		.func_handler  = &stdout_handler
	}
};

backend_t stderr_proto = {
	.class          = "io/stderr",
	.supported_api  = API_HASH,
	.func_init      = &stdout_init,
	.func_configure = &stdout_configure,
	.func_destroy   = &stdout_destroy,
	.backend_type_hash = {
		.func_handler  = &stderr_handler
	}
};
