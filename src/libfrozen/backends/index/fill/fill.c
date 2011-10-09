#include <libfrozen.h>

/**
 * @ingroup modules
 * @addtogroup mod_backend_fill Module 'index/fill'
 */
/**
 * @ingroup mod_backend_fill
 * @page page_fill_info Description
 *
 * This module fill index with data, or delete it
 */
/**
 * @ingroup mod_backend_fill
 * @page page_fill_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "index/fill",
 *              action                  =                     # action for index request
 *                                        "create"            #  - create new item in index
 *                                        "write"             #  - update item value, or create new item with this value (!)
 *                                        "delete"            #  - delete item from index (need parent module with action == DELETE check for ex.)
 *                                        ...
 *              index                   = 
 *                                        "index_name",       # existing index
 *                                        { ... },            # new index configuration
 *              fatal                   = (uint_t)'1',        # don't ignore errors from index, default 0
 *              before                  = (uint_t)'1'         # query index before passing request, default 1 for action=="delete", 0 for others
 *                                                            # this value override any default
 * }
 * @endcode
 */
/**
 * @ingroup mod_backend_fill
 * @page page_fill_io Input and output
 * 
 * Send every request to index with defined action after\before passing it to underlying backend.
 *
 */

#define EMODULE 29

typedef struct fill_userdata {
	uintmax_t              fatal;
	uintmax_t              before;
	data_functions         action;
	backend_t             *backend_index;
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
	ssize_t                ret;
	char                  *action_str        = "write";
	fill_userdata         *userdata          = (fill_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT,  action_str,              config, HK(action));
	hash_data_copy(ret, TYPE_UINTT,    userdata->fatal,         config, HK(fatal));
	hash_data_copy(ret, TYPE_BACKENDT, userdata->backend_index, config, HK(index));
	if(ret != 0)
		return error("supplied index backend not valid, or not found");
	
	userdata->action = request_str_to_action(action_str);
	
	if(userdata->action == ACTION_DELETE)
		userdata->before = 1; // set default value
	
	hash_data_copy(ret, TYPE_UINTT,    userdata->before,        config, HK(before));
	return 0;
} // }}}

static ssize_t fill_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t              ret, ret2, ret3;
	fill_userdata       *userdata            = (fill_userdata *)backend->userdata;
	
	if( userdata->before == 0 )
		ret = backend_pass(backend, request);
	
	// data = hash_data_find( request, userdata->output );
	// if (data->flags & MODIFIED && userdata->force_update == 0) {
		request_t r_update[] = {
			{ HK(action), DATA_UINT32T(userdata->action) },
			{ HK(ret),    DATA_PTR_SIZET(&ret3)          },
			hash_next(request)
		};
		ret2 = backend_query(userdata->backend_index, r_update);
		if(userdata->fatal != 0){
			if(ret2 != 0) return ret2;
			if(ret3 != 0) return ret3;
		}
	// }
	
	if( userdata->before != 0 )
		ret = backend_pass(backend, request);
	
	return (ret < 0) ? ret : -EEXIST;
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
