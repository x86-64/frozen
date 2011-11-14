#include <libfrozen.h>
#include <dataproto.h>

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

typedef struct std_userdata {
	hash_key_t             key;
} std_userdata;

static ssize_t stdin_io_handler(data_t *data, FILE **fd, fastcall_header *hargs){ // {{{
	switch(hargs->action){
		case ACTION_READ:;
			fastcall_read *rargs = (fastcall_read *)hargs;
			
			if(feof(*fd))
				return -1;
			
			rargs->buffer_size = fread(rargs->buffer, 1, rargs->buffer_size, *fd);
			return 0;
		
		case ACTION_TRANSFER:
			return data_protos[ TYPE_DEFAULTT ]->handlers[ ACTION_TRANSFER ](data, hargs);

		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t stdout_io_handler(data_t *data, FILE **fd, fastcall_header *hargs){ // {{{
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

static data_t stdin_io  = DATA_IOT(&stdin,  (f_io_func)&stdin_io_handler);
static data_t stdout_io = DATA_IOT(&stdout, (f_io_func)&stdout_io_handler);
static data_t stderr_io = DATA_IOT(&stderr, (f_io_func)&stdout_io_handler);

static int std_init(backend_t *backend){ // {{{
	std_userdata        *userdata;

	if((userdata = backend->userdata = calloc(1, sizeof(std_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->key       = HK(buffer);
	return 0;
} // }}}
static int std_destroy(backend_t *backend){ // {{{
	std_userdata          *userdata          = (std_userdata *)backend->userdata;

	free(userdata);
	return 0;
} // }}}

static int stdin_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	std_userdata          *userdata          = (std_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->key,      config, HK(output));
	return 0;
} // }}}
static int stdout_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	std_userdata          *userdata          = (std_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->key,      config, HK(input));
	return 0;
} // }}}

static ssize_t stdin_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret, retd;
	data_t                *output;
	std_userdata          *userdata          = (std_userdata *)backend->userdata;
	
	if( (output = hash_data_find(request, userdata->key)) == NULL)
		return error("output key not supplied");
	
	fastcall_transfer r_transfer = { { 4, ACTION_TRANSFER }, output };
	ret = data_query(&stdin_io, &r_transfer);
	
	hash_data_set(retd, TYPE_UINTT, r_transfer.transfered, request, HK(size));
	return ret;
} // }}}
static ssize_t stdout_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret, retd;
	data_t                *input;
	std_userdata          *userdata          = (std_userdata *)backend->userdata;
	
	if( (input = hash_data_find(request, userdata->key)) == NULL)
		return error("input key not supplied");
	
	static fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &stdout_io };
	ret = data_query(input, &r_transfer);
	
	hash_data_set(retd, TYPE_UINTT, r_transfer.transfered, request, HK(size));
	return ret;
} // }}}
static ssize_t stderr_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret, retd;
	data_t                *input;
	std_userdata          *userdata          = (std_userdata *)backend->userdata;
	
	if( (input = hash_data_find(request, userdata->key)) == NULL)
		return error("input key not supplied");
	
	static fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &stderr_io };
	ret = data_query(input, &r_transfer);
	
	hash_data_set(retd, TYPE_UINTT, r_transfer.transfered, request, HK(size));
	return ret;
} // }}}

backend_t stdin_proto = {
	.class          = "io/stdin",
	.supported_api  = API_HASH,
	.func_init      = &std_init,
	.func_configure = &stdin_configure,
	.func_destroy   = &std_destroy,
	.backend_type_hash = {
		.func_handler  = &stdin_handler
	}
};

backend_t stdout_proto = {
	.class          = "io/stdout",
	.supported_api  = API_HASH,
	.func_init      = &std_init,
	.func_configure = &stdout_configure,
	.func_destroy   = &std_destroy,
	.backend_type_hash = {
		.func_handler  = &stdout_handler
	}
};

backend_t stderr_proto = {
	.class          = "io/stderr",
	.supported_api  = API_HASH,
	.func_init      = &std_init,
	.func_configure = &stdout_configure,
	.func_destroy   = &std_destroy,
	.backend_type_hash = {
		.func_handler  = &stderr_handler
	}
};
