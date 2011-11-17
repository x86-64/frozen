#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_implode request/implode
 */
/**
 * @ingroup mod_backend_implode
 * @page page_implode_info Description
 *
 * This backend pack current request into one key-value pair in new request for underlying backend. Useful for ipcs, or
 * for request storage. See @ref mod_backend_explode for unpacking.
 *
 * This backend actually do nothing with request, just wrapping. Underlying backend must do all job with converting to packed
 * state. "communication/ipc" already does it.
 */
/**
 * @ingroup mod_backend_implode
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
 * @ingroup backend
 * @addtogroup mod_backend_explode request/explode
 */
/**
 * @ingroup mod_backend_explode
 * @page page_explode_info Description
 *
 * This backend unpack new request from current request. Useful for ipc. See @ref mod_backend_implode for packing.
 *
 * Unpacking process use data _CONVERT_FROM functions, and _CONVERT_TO for result returning. All data types used in request
 * have to support them.
 */
/**
 * @ingroup mod_backend_explode
 * @page page_explode_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "request/explode",
 *              buffer                  = (hashkey_t)'buffer',  # key name in new request
 *              return_result           = (uint_t)'0',          # pack changed, during further processing, request back to old one, default 1
 * }
 * @endcode
 */

#define EMODULE 34

typedef struct plode_userdata {
	hash_key_t             buffer;
	uintmax_t              return_result;
} plode_userdata;

static int plode_init(backend_t *backend){ // {{{
	plode_userdata        *userdata          = backend->userdata = calloc(1, sizeof(plode_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->buffer        = HK(buffer);
	userdata->return_result = 1;
	return 0;
} // }}}
static int plode_destroy(backend_t *backend){ // {{{
	plode_userdata          *userdata          = (plode_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int plode_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	plode_userdata        *userdata          = (plode_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->buffer,        config, HK(buffer));
	hash_data_copy(ret, TYPE_UINTT,    userdata->return_result, config, HK(return_result));
	return 0;
} // }}}

static ssize_t implode_request(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	plode_userdata        *userdata          = (plode_userdata *)backend->userdata;
	
	request_t r_next[] = {
		{ userdata->buffer, DATA_PTR_HASHT(request) },
		hash_end
	};
	return ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
} // }}}
static ssize_t explode_request(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *buffer;
	data_t                 r_hash            = DATA_PTR_HASHT(NULL);
	plode_userdata        *userdata          = (plode_userdata *)backend->userdata;
	
	if( (buffer = hash_data_find(request, userdata->buffer)) == NULL)
		return -EINVAL;
	
	fastcall_convert_from  r_convert_from = { { 4, ACTION_CONVERT_FROM }, buffer, FORMAT_BINARY };
	if( (ret = data_query(&r_hash, &r_convert_from)) < 0)
		return -EFAULT;
	
	if(r_hash.ptr == NULL)
		return -EFAULT;
	
	ret = ( (ret = backend_pass(backend, r_hash.ptr)) < 0) ? ret : -EEXIST;
	
	if(userdata->return_result != 0){
		fastcall_convert_to    r_convert_to = { { 4, ACTION_CONVERT_TO }, buffer, FORMAT_BINARY };
		if( (ret = data_query(&r_hash, &r_convert_to)) < 0)
			return -EFAULT;
	}
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&r_hash, &r_free);
	
	return ret;
} // }}}

backend_t implode_proto = {
	.class          = "request/implode",
	.supported_api  = API_HASH,
	.func_init      = &plode_init,
	.func_configure = &plode_configure,
	.func_destroy   = &plode_destroy,
	.backend_type_hash = {
		.func_handler = &implode_request,
	}
};

backend_t explode_proto = {
	.class          = "request/explode",
	.supported_api  = API_HASH,
	.func_init      = &plode_init,
	.func_configure = &plode_configure,
	.func_destroy   = &plode_destroy,
	.backend_type_hash = {
		.func_handler = &explode_request,
	}
};

