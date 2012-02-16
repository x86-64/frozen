#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_implode request/implode
 */
/**
 * @ingroup mod_machine_implode
 * @page page_implode_info Description
 *
 * This machine pack current request into one key-value pair in new request for underlying machine. Useful for ipcs, or
 * for request storage. See @ref mod_machine_try for unpacking.
 *
 * This machine actually do nothing with request, just wrapping. Underlying machine must do all job with converting to packed
 * state. "communication/ipc" already does it.
 */
/**
 * @ingroup mod_machine_implode
 * @page page_implode_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "request/implode",
 *              buffer                  = (hashkey_t)'buffer',  # key name in new request
 * }
 * @endcode
 */

#define EMODULE 34

typedef struct implode_userdata {
	hashkey_t              buffer;
} implode_userdata;

static int implode_init(machine_t *machine){ // {{{
	implode_userdata      *userdata          = machine->userdata = calloc(1, sizeof(implode_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->buffer        = HK(buffer);
	return 0;
} // }}}
static int implode_destroy(machine_t *machine){ // {{{
	implode_userdata      *userdata          = (implode_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int implode_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	implode_userdata      *userdata          = (implode_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->buffer,        config, HK(buffer));
	return 0;
} // }}}

static ssize_t implode_request(machine_t *machine, request_t *request){ // {{{
	implode_userdata      *userdata          = (implode_userdata *)machine->userdata;
	
	request_t r_next[] = {
		{ userdata->buffer, DATA_PTR_HASHT(request) },
		hash_inline(request),
		hash_end
	};
	return machine_pass(machine, r_next);
} // }}}

machine_t implode_proto = { // {{{
	.class          = "request/implode",
	.supported_api  = API_HASH,
	.func_init      = &implode_init,
	.func_configure = &implode_configure,
	.func_destroy   = &implode_destroy,
	.machine_type_hash = {
		.func_handler = &implode_request,
	}
}; // }}}

