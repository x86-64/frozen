#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_lookup index/lookup
 */
/**
 * @ingroup mod_backend_lookup
 * @page page_lookup_info Description
 *
 * This module query supplied index for key, and pass request to underlying backends with returned value
 */
/**
 * @ingroup mod_backend_lookup
 * @page page_lookup_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "index/lookup",
 *              index                   = 
 *                                        "index_name",       # existing index
 *                                        { ... },            # new index configuration
 * }
 * @endcode
 */

#define EMODULE 21

typedef struct lookup_userdata {
	backend_t             *backend_index;
} lookup_userdata;

static int lookup_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(lookup_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int lookup_destroy(backend_t *backend){ // {{{
	lookup_userdata       *userdata = (lookup_userdata *)backend->userdata;
	
	if(userdata->backend_index != NULL)
		backend_destroy(userdata->backend_index);

	free(userdata);
	return 0;
} // }}}
static int lookup_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	lookup_userdata       *userdata          = (lookup_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_BACKENDT,  userdata->backend_index, config, HK(index));
	if(ret != 0)
		return error("supplied index backend not valid, or not found");
	
	return 0;
} // }}}

static ssize_t lookup_handler(backend_t *backend, request_t *request){ // {{{
	lookup_userdata       *userdata          = (lookup_userdata *)backend->userdata;
	
	request_t r_query[] = {
		{ HK(action),          DATA_UINT32T(ACTION_READ) },
		{ HK(return_to),       DATA_BACKENDT(backend)    },
		hash_next(request)
	};
	return backend_query(userdata->backend_index, r_query);
} // }}}

backend_t lookup_proto = {
	.class          = "index/lookup",
	.supported_api  = API_HASH,
	.func_init      = &lookup_init,
	.func_destroy   = &lookup_destroy,
	.func_configure = &lookup_configure,
	.backend_type_hash = {
		.func_handler = &lookup_handler
	}
};
