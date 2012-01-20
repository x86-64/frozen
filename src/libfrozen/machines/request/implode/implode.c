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
 * for request storage. See @ref mod_machine_explode for unpacking.
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

/**
 * @ingroup machine
 * @addtogroup mod_machine_explode request/explode
 */
/**
 * @ingroup mod_machine_explode
 * @page page_explode_info Description
 *
 * This machine unpack new request from current request. Useful for ipc. See @ref mod_machine_implode for packing.
 *
 * Unpacking process use data _CONVERT_FROM functions. All data types used in request have to support it.
 */
/**
 * @ingroup mod_machine_explode
 * @page page_explode_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "request/explode",
 *              buffer                  = (hashkey_t)'buffer',  # key name in new request
 * }
 * @endcode
 */

#define EMODULE 34

typedef struct plode_userdata {
	hashkey_t              buffer;
} plode_userdata;

static int plode_init(machine_t *machine){ // {{{
	plode_userdata        *userdata          = machine->userdata = calloc(1, sizeof(plode_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->buffer        = HK(buffer);
	return 0;
} // }}}
static int plode_destroy(machine_t *machine){ // {{{
	plode_userdata          *userdata          = (plode_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int plode_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	plode_userdata        *userdata          = (plode_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->buffer,        config, HK(buffer));
	return 0;
} // }}}

static ssize_t implode_request(machine_t *machine, request_t *request){ // {{{
	plode_userdata        *userdata          = (plode_userdata *)machine->userdata;
	
	request_t r_next[] = {
		{ userdata->buffer, DATA_PTR_HASHT(request) },
		hash_end
	};
	return machine_pass(machine, r_next);
} // }}}
static ssize_t explode_request(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *buffer;
	data_t                 r_hash            = DATA_PTR_HASHT(NULL);
	plode_userdata        *userdata          = (plode_userdata *)machine->userdata;
	
	if( (buffer = hash_data_find(request, userdata->buffer)) == NULL)
		return -EINVAL;
	
	fastcall_convert_from  r_convert_from = { { 4, ACTION_CONVERT_FROM }, buffer, FORMAT(binary) };
	if( (ret = data_query(&r_hash, &r_convert_from)) < 0)
		return -EFAULT;
	
	if(r_hash.ptr == NULL)
		return -EFAULT;
	
	ret = machine_pass(machine, r_hash.ptr);
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&r_hash, &r_free);
	
	return ret;
} // }}}

machine_t implode_proto = {
	.class          = "request/implode",
	.supported_api  = API_HASH,
	.func_init      = &plode_init,
	.func_configure = &plode_configure,
	.func_destroy   = &plode_destroy,
	.machine_type_hash = {
		.func_handler = &implode_request,
	}
};

machine_t explode_proto = {
	.class          = "request/explode",
	.supported_api  = API_HASH,
	.func_init      = &plode_init,
	.func_configure = &plode_configure,
	.func_destroy   = &plode_destroy,
	.machine_type_hash = {
		.func_handler = &explode_request,
	}
};

