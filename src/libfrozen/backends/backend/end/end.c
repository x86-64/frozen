#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_end backend/end
 */
/**
 * @ingroup mod_backend_end
 * @page page_end_info Description
 *
 * This backend terminate request with given error code. Can be useful in 'switch' rules
 */
/**
 * @ingroup mod_backend_end
 * @page page_end_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "backend/end",
 *              return                  = (uint_t)'0'          # return this error code, default 0 (no error)
 * }
 * @endcode
 */

#define EMODULE 37

typedef struct end_userdata {
	uintmax_t              ret;
} end_userdata;

static int end_init(backend_t *backend){ // {{{
	end_userdata         *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(end_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->ret = 0;
	return 0;
} // }}}
static int end_destroy(backend_t *backend){ // {{{
	end_userdata      *userdata = (end_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int end_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	end_userdata          *userdata          = (end_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_UINTT,    userdata->ret, config, HK(return));
	return 0;
} // }}}

static ssize_t end_handler(backend_t *backend, request_t *request){ // {{{
	end_userdata          *userdata          = (end_userdata *)backend->userdata;
	
	return userdata->ret;
} // }}}
static ssize_t end_fast_handler(backend_t *backend, void *hargs){ // {{{
	end_userdata          *userdata          = (end_userdata *)backend->userdata;
	
	return userdata->ret;
} // }}}


backend_t end_proto = {
	.class          = "backend/end",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = &end_init,
	.func_destroy   = &end_destroy,
	.func_configure = &end_configure,
	.backend_type_hash = {
		.func_handler = &end_handler
	},
	.backend_type_fast = {
		.func_handler = &end_fast_handler
	}
};

