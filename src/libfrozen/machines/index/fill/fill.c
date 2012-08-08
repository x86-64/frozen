#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_fill index/fill
 */
/**
 * @ingroup mod_machine_fill
 * @page page_fill_info Description
 *
 * This module fill index with data, or delete it
 */
/**
 * @ingroup mod_machine_fill
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
 * }
 * @endcode
 */
/**
 * @ingroup mod_machine_fill
 * @page page_fill_io Input and output
 * 
 * Send every request to index with defined action after\before passing it to underlying machine.
 *
 */

#define ERRORS_MODULE_ID 29
#define ERRORS_MODULE_NAME "index/fill"

typedef struct fill_userdata {
	uintmax_t              fatal;
	action_t               action;
	machine_t             *machine_index;
} fill_userdata;

static ssize_t fill_init(machine_t *machine){ // {{{
	fill_userdata         *userdata;

	if((userdata = machine->userdata = calloc(1, sizeof(fill_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->action = ACTION_WRITE;
	return 0;
} // }}}
static ssize_t fill_destroy(machine_t *machine){ // {{{
	fill_userdata       *userdata = (fill_userdata *)machine->userdata;
	
	pipeline_destroy(userdata->machine_index);
	free(userdata);
	return 0;
} // }}}
static ssize_t fill_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	fill_userdata         *userdata          = (fill_userdata *)machine->userdata;
	
	hash_data_get    (ret, TYPE_ACTIONT,  userdata->action,        config, HK(action));
	hash_data_get    (ret, TYPE_UINTT,    userdata->fatal,         config, HK(fatal));
	hash_data_consume(ret, TYPE_MACHINET, userdata->machine_index, config, HK(index));
	if(ret != 0)
		return error("supplied index machine not valid, or not found");
	
	return 0;
} // }}}

static ssize_t fill_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t              ret2, ret3;
	fill_userdata       *userdata            = (fill_userdata *)machine->userdata;
	
	// data = hash_data_find( request, userdata->output );
	// if (data->flags & MODIFIED && userdata->force_update == 0) {
		request_t r_update[] = {
			{ HK(action), DATA_ACTIONT(userdata->action) },
			{ HK(ret),    DATA_SIZET(0)                  },
			hash_next(request)
		};
		ret2 = machine_query(userdata->machine_index, r_update);
		ret3 = DEREF_TYPE_SIZET(&r_update[1].data);
		if(userdata->fatal != 0){
			if(ret2 != 0) return ret2;
			if(ret3 != 0) return ret3;
		}
	// }
	
	return machine_pass(machine, request);
} // }}}

machine_t fill_proto = {
	.class          = "index/fill",
	.supported_api  = API_HASH,
	.func_init      = &fill_init,
	.func_destroy   = &fill_destroy,
	.func_configure = &fill_configure,
	.machine_type_hash = {
		.func_handler = &fill_handler
	}
};
