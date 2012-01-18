#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_end request/end
 */
/**
 * @ingroup mod_machine_end
 * @page page_end_info Description
 *
 * This machine terminate request with given error code. Can be useful in 'switch' rules
 */
/**
 * @ingroup mod_machine_end
 * @page page_end_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "request/end",
 *              return                  = (uint_t)'0'          # return this error code, default 0 (no error)
 * }
 * @endcode
 */

#define EMODULE 37

typedef struct end_userdata {
	uintmax_t              ret;
} end_userdata;

static int end_init(machine_t *machine){ // {{{
	end_userdata         *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(end_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->ret = 0;
	return 0;
} // }}}
static int end_destroy(machine_t *machine){ // {{{
	end_userdata      *userdata = (end_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int end_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	end_userdata          *userdata          = (end_userdata *)machine->userdata;
	
	hash_data_copy(ret, TYPE_UINTT,    userdata->ret, config, HK(return));
	return 0;
} // }}}

static ssize_t end_handler(machine_t *machine, request_t *request){ // {{{
	end_userdata          *userdata          = (end_userdata *)machine->userdata;
	
	return userdata->ret;
} // }}}

machine_t end_proto = {
	.class          = "request/end",
	.supported_api  = API_HASH,
	.func_init      = &end_init,
	.func_destroy   = &end_destroy,
	.func_configure = &end_configure,
	.machine_type_hash = {
		.func_handler = &end_handler
	},
};

