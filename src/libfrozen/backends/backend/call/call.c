#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_call Backend 'backend/call'
 */
/**
 * @ingroup mod_backend_call
 * @page page_call_info Description
 *
 * This backend call user-supplied backend with current request. If user give empty backend value, then
 * request passed to underlying backend, and then "call" make one more attempt.
 *
 * Underlying backend is expected to be backend-creating, but this is optional.
 */
/**
 * @ingroup mod_backend_call
 * @page page_call_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "backend/call",
 *              backend                 = (hashkey_t)'name',  # key with backend in request, default "backend"
 *              retry                   = (uint_t)'1',        # rerun request after failed call, default 1
 * }
 * @endcode
 */

#define EMODULE 25

typedef struct call_userdata {
	uintmax_t              retry_request;
	hash_key_t             hk_backend;
} call_userdata;

static int call_init(backend_t *backend){ // {{{
	call_userdata         *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(call_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->hk_backend    = HK(backend);
	userdata->retry_request = 1;
	return 0;
} // }}}
static int call_destroy(backend_t *backend){ // {{{
	call_userdata      *userdata = (call_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int call_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	call_userdata         *userdata          = (call_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->hk_backend,    config, HK(input));
	hash_data_copy(ret, TYPE_UINTT,    userdata->retry_request, config, HK(retry));
	return 0;
} // }}}

static ssize_t call_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	backend_t             *call_to           = NULL;
	call_userdata         *userdata          = (call_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_BACKENDT, call_to, request, userdata->hk_backend);
	if(call_to)
		return ( (ret = backend_query(call_to, request)) < 0 ) ? ret : -EEXIST;
	
	ret = ( (ret = backend_pass(backend, request)) < 0 ) ? ret : -EEXIST;
	
	if(userdata->retry_request != 0){
		hash_data_copy(ret, TYPE_BACKENDT, call_to, request, userdata->hk_backend);
		if(call_to)
			return ( (ret = backend_query(call_to, request)) < 0 ) ? ret : -EEXIST;
	}
	return ret;
} // }}}

backend_t call_proto = {
	.class          = "backend/call",
	.supported_api  = API_HASH,
	.func_init      = &call_init,
	.func_destroy   = &call_destroy,
	.func_configure = &call_configure,
	.backend_type_hash = {
		.func_handler = &call_handler
	}
};

