#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_assign request/assign
 *
 * Assign module set new request's keys.
 */
/**
 * @ingroup mod_backend_assign
 * @page page_assign_config Configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class      = "request/assign",
 * 	        before     = {                            # this key-values override existing key-values with same key, optional
 *                      buffer = (string_t)'hello'
 * 	        },
 * 	        after      = {                            # this key-values would be used if there is no other with such key, optional
 *                      buffer = (string_t)'hello'
 * 	        },
 * 	        request    = {                            # override whole request with this one (before and after not taken in account), optional
 *                      buffer = (string_t)'hello'
 * 	        }
 * 	}
 * @endcode
 */

#define EMODULE         43

typedef struct assign_userdata {
	hash_t                *before;
	hash_t                *after;
	hash_t                *request;
} assign_userdata;

static int assign_init(backend_t *backend){ // {{{
	if( (backend->userdata = calloc(1, sizeof(assign_userdata))) == NULL)
		return error("calloc returns null");
	
	return 0;
} // }}}
static int assign_destroy(backend_t *backend){ // {{{
	assign_userdata       *userdata          = (assign_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int assign_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	assign_userdata       *userdata          = (assign_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHT, userdata->before,   config, HK(before));
	hash_data_copy(ret, TYPE_HASHT, userdata->after,    config, HK(after));
	hash_data_copy(ret, TYPE_HASHT, userdata->request,  config, HK(request));
	return 0;
} // }}}

static ssize_t assign_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	assign_userdata       *userdata          = (assign_userdata *)backend->userdata;
	
	if(userdata->request != NULL){
		return ( (ret = backend_pass(backend, userdata->request)) < 0 ) ? ret : -EEXIST;
	}else{
		hash_t r_next[] = {
			hash_inline(userdata->before),
			hash_inline(request),
			hash_inline(userdata->after),
			hash_end
		};
		
		return ( (ret = backend_pass(backend, r_next)) < 0 ) ? ret : -EEXIST;
	}
} // }}}

backend_t assign_proto = {
	.class          = "request/assign",
	.supported_api  = API_HASH,
	.func_init      = &assign_init,
	.func_configure = &assign_configure,
	.func_destroy   = &assign_destroy,
	.backend_type_hash = {
		.func_handler = &assign_handler
	}
};

