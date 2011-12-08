#include <libfrozen.h>
#include <murmur2.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_murmur hash/murmur
 */
/**
 * @ingroup mod_backend_murmur
 * @page page_murmur_info Description
 *
 * This is implementation of murmur2 hash. It hash input data and add resulting hash to request.
 */
/**
 * @ingroup mod_backend_murmur
 * @page page_murmur_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = 
 *                                        "hash/murmur2_32",
 *                                        "hash/murmur2_64",
 *              input                   = (hashkey_t)'buffer', # input key name
 *              output                  = (hashkey_t)'keyid',  # output key name
 *              fatal                   = (uint_t)'1',         # interrupt request if input not present, default 0
 * }
 * @endcode
 */

#define EMODULE 22

typedef struct murmur_userdata {
	hashkey_t             input;
	hashkey_t             output;
	uintmax_t              fatal;
} murmur_userdata;

static int murmur_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(murmur_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int murmur_destroy(backend_t *backend){ // {{{
	murmur_userdata       *userdata = (murmur_userdata *)backend->userdata;

	free(userdata);
	return 0;
} // }}}
static int murmur_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	murmur_userdata       *userdata          = (murmur_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->input,    config, HK(input));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->output,   config, HK(output));
	hash_data_copy(ret, TYPE_UINTT,    userdata->fatal,    config, HK(fatal));
	
	userdata->fatal   = ( userdata->fatal == 0 ) ? 0 : 1;
	return 0;
} // }}}

static ssize_t murmur2_32_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uint32_t               hash              = 0;
	data_t                *key               = NULL;
	murmur_userdata       *userdata          = (murmur_userdata *)backend->userdata;
	
	key = hash_data_find(request, userdata->input);
	if(key == NULL){
		if(userdata->fatal == 0)
			return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
		return error("input key not supplied");
	}
	
	hash = MurmurHash2(key, 0);
	
	request_t r_next[] = {
		{ userdata->output, DATA_PTR_UINT32T(&hash) },
		hash_next(request)
	};
	return ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
} // }}}
static ssize_t murmur2_64_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uint64_t               hash              = 0;
	data_t                *key               = NULL;
	murmur_userdata       *userdata          = (murmur_userdata *)backend->userdata;
	
	key = hash_data_find(request, userdata->input);
	if(key == NULL){
		if(userdata->fatal == 0)
			return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
		return error("input key not supplied");
	}
	
	hash = MurmurHash64A(key, 0);
	
	request_t r_next[] = {
		{ userdata->output, DATA_PTR_UINT64T(&hash) },
		hash_next(request)
	};
	return ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
} // }}}

backend_t murmur2_32_proto = {
	.class          = "hash/murmur2_32",
	.supported_api  = API_HASH,
	.func_init      = &murmur_init,
	.func_destroy   = &murmur_destroy,
	.func_configure = &murmur_configure,
	.backend_type_hash = {
		.func_handler  = &murmur2_32_handler
	}
};

backend_t murmur2_64_proto = {
	.class          = "hash/murmur2_64",
	.supported_api  = API_HASH,
	.func_init      = &murmur_init,
	.func_destroy   = &murmur_destroy,
	.func_configure = &murmur_configure,
	.backend_type_hash = {
		.func_handler  = &murmur2_64_handler
	}
};

