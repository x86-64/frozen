#include <libfrozen.h>

/**
 * @ingroup modules
 * @addtogroup mod_backend_fill Module 'index/fill'
 */
/**
 * @ingroup mod_backend_fill
 * @page page_fill_info Description
 *
 * This module fills index with data
 */
/**
 * @ingroup mod_backend_fill
 * @page page_fill_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "index/fill",
 *              index                   = 
 *                                        "index_name",       # existing index
 *                                        { ... },            # new index configuration
 * }
 * @endcode
 */
/**
 * @ingroup mod_backend_fill
 * @page page_fill_io Input and output
 * 
 *
 */

#define EMODULE 29

typedef struct fill_userdata {
} fill_userdata;

static int fill_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(fill_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int fill_destroy(backend_t *backend){ // {{{
	fill_userdata       *userdata = (fill_userdata *)backend->userdata;
	
	if(userdata->backend_index != NULL)
		backend_destroy(userdata->backend_index);

	free(userdata);
	return 0;
} // }}}
static int fill_configure(backend_t *backend, config_t *config){ // {{{
	//ssize_t                ret;
	//fill_userdata         *userdata          = (fill_userdata *)backend->userdata;
	
	return 0;
} // }}}

static ssize_t fill_handler(backend_t *backend, request_t *request){ // {{{
	return -EINVAL;
} // }}}

backend_t fill_proto = {
	.class          = "index/fill",
	.supported_api  = API_HASH,
	.func_init      = &fill_init,
	.func_destroy   = &fill_destroy,
	.func_configure = &fill_configure,
	.backend_type_hash = {
		.func_handler = &fill_handler
	}
};
