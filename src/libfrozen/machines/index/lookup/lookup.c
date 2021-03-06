#include <libfrozen.h>
#include <shop/pass/pass.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_lookup index/lookup
 */
/**
 * @ingroup mod_machine_lookup
 * @page page_lookup_info Description
 *
 * This module query supplied index for key, and pass request to underlying machines with returned value
 */
/**
 * @ingroup mod_machine_lookup
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

#define ERRORS_MODULE_ID 21
#define ERRORS_MODULE_NAME "index/lookup"

typedef struct lookup_userdata {
	machine_t             *machine_index;
} lookup_userdata;

static ssize_t lookup_init(machine_t *machine){ // {{{
	if((machine->userdata = calloc(1, sizeof(lookup_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static ssize_t lookup_destroy(machine_t *machine){ // {{{
	lookup_userdata       *userdata = (lookup_userdata *)machine->userdata;
	
	pipeline_destroy(userdata->machine_index);
	free(userdata);
	return 0;
} // }}}
static ssize_t lookup_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	lookup_userdata       *userdata          = (lookup_userdata *)machine->userdata;
	
	hash_data_consume(ret, TYPE_MACHINET,  userdata->machine_index, config, HK(index));
	if(ret != 0)
		return error("supplied index machine not valid, or not found");
	
	return 0;
} // }}}

static ssize_t lookup_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	lookup_userdata       *userdata          = (lookup_userdata *)machine->userdata;
	
	stack_call(machine->cnext);

		request_t r_query[] = {
			{ HK(action),          DATA_ACTIONT(ACTION_READ) },
			hash_next(request)
		};
		ret = machine_query(userdata->machine_index, r_query);
		
	stack_clean();
	return ret;
} // }}}

machine_t lookup_proto = {
	.class          = "index/lookup",
	.supported_api  = API_HASH,
	.func_init      = &lookup_init,
	.func_destroy   = &lookup_destroy,
	.func_configure = &lookup_configure,
	.machine_type_hash = {
		.func_handler = &lookup_handler
	}
};
