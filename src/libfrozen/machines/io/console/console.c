#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_stdin io/stdin
 */
/**
 * @ingroup mod_machine_stdin
 * @page page_stdin_info Description
 *
 * This machine read from console. It is passive module, use @ref mod_machine_transfer to read and pass input to another module.
 */
/**
 * @ingroup mod_machine_stdin
 * @page page_stdin_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "io/stdin",
 *              output                  = (hashkey_t)'buffer'     # output buffer key, default "buffer"
 * }
 * @endcode
 */

/**
 * @ingroup machine
 * @addtogroup mod_machine_stdout io/stdout
 */
/**
 * @ingroup mod_machine_stdout
 * @page page_stdout_info Description
 *
 * This machine write to console stdout
 */
/**
 * @ingroup mod_machine_stdout
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
 * @ingroup machine
 * @addtogroup mod_machine_stderr io/stderr
 */
/**
 * @ingroup mod_machine_stderr
 * @page page_stderr_info Description
 *
 * This machine write to console stderr
 */
/**
 * @ingroup mod_machine_stderr
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

#define ERRORS_MODULE_ID 38
#define ERRORS_MODULE_NAME "io/console"

typedef struct std_userdata {
	hashkey_t             key;
} std_userdata;

static ssize_t stdin_io_handler(data_t *data, FILE **fd, fastcall_header *hargs){ // {{{
	switch(hargs->action){
		case ACTION_READ:;
			fastcall_read *rargs = (fastcall_read *)hargs;
			
			if(feof(*fd))
				return -1;
			
			rargs->buffer_size = fread(rargs->buffer, 1, rargs->buffer_size, *fd);
			return 0;
		
		case ACTION_CONVERT_TO:
			return data_default_convert_to(data, (fastcall_convert_to *)hargs);

		default:
			break;
	}
	return errorn(ENOSYS);
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
	return errorn(ENOSYS);
} // }}}

static data_t stdin_io  = DATA_IOT(&stdin,  (f_io_func)&stdin_io_handler);
static data_t stdout_io = DATA_IOT(&stdout, (f_io_func)&stdout_io_handler);
static data_t stderr_io = DATA_IOT(&stderr, (f_io_func)&stdout_io_handler);

static ssize_t std_init(machine_t *machine){ // {{{
	std_userdata        *userdata;

	if((userdata = machine->userdata = calloc(1, sizeof(std_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->key       = HK(buffer);
	return 0;
} // }}}
static ssize_t std_destroy(machine_t *machine){ // {{{
	std_userdata          *userdata          = (std_userdata *)machine->userdata;

	free(userdata);
	return 0;
} // }}}

static ssize_t stdin_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	std_userdata          *userdata          = (std_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->key,      config, HK(output));
	return 0;
} // }}}
static ssize_t stdout_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	std_userdata          *userdata          = (std_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->key,      config, HK(input));
	return 0;
} // }}}

static ssize_t stdin_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret, retd;
	data_t                *output;
	std_userdata          *userdata          = (std_userdata *)machine->userdata;
	
	if( (output = hash_data_find(request, userdata->key)) == NULL)
		return error("output key not supplied");
	
	fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_TO }, output, FORMAT(native) };
	ret = data_query(&stdin_io, &r_convert);
	
	hash_data_set(retd, TYPE_UINTT, r_convert.transfered, request, HK(size));
	return ret;
} // }}}
static ssize_t stdout_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret, retd;
	data_t                *input;
	std_userdata          *userdata          = (std_userdata *)machine->userdata;
	
	if( (input = hash_data_find(request, userdata->key)) == NULL)
		return error("input key not supplied");
	
	static fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, &stdout_io, FORMAT(native) };
	ret = data_query(input, &r_convert);
	
	hash_data_set(retd, TYPE_UINTT, r_convert.transfered, request, HK(size));
	return ret;
} // }}}
static ssize_t stderr_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret, retd;
	data_t                *input;
	std_userdata          *userdata          = (std_userdata *)machine->userdata;
	
	if( (input = hash_data_find(request, userdata->key)) == NULL)
		return error("input key not supplied");
	
	static fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, &stderr_io, FORMAT(native) };
	ret = data_query(input, &r_convert);
	
	hash_data_set(retd, TYPE_UINTT, r_convert.transfered, request, HK(size));
	return ret;
} // }}}

machine_t stdin_proto = {
	.class          = "io/stdin",
	.supported_api  = API_HASH,
	.func_init      = &std_init,
	.func_configure = &stdin_configure,
	.func_destroy   = &std_destroy,
	.machine_type_hash = {
		.func_handler  = &stdin_handler
	}
};

machine_t stdout_proto = {
	.class          = "io/stdout",
	.supported_api  = API_HASH,
	.func_init      = &std_init,
	.func_configure = &stdout_configure,
	.func_destroy   = &std_destroy,
	.machine_type_hash = {
		.func_handler  = &stdout_handler
	}
};

machine_t stderr_proto = {
	.class          = "io/stderr",
	.supported_api  = API_HASH,
	.func_init      = &std_init,
	.func_configure = &stdout_configure,
	.func_destroy   = &std_destroy,
	.machine_type_hash = {
		.func_handler  = &stderr_handler
	}
};
