#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_query data/query
 */
/**
 * @ingroup mod_machine_query
 * @page page_query_info Description
 *
 * This machine makes query to supplied data and pass result to next machine.
 */
/**
 * @ingroup mod_machine_query
 * @page page_query_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/query",
 *              data                    =                        # data to query
 *                                        (env_t)"data",         # - query data supplied in request
 *                                        (sometype_t)"",        # - custom data
 *                                        ....
 * }
 * @endcode
 */

#define EMODULE 52

typedef struct query_userdata {
	data_t                *data;
} query_userdata;

static int query_init(machine_t *machine){ // {{{
	query_userdata       *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(query_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int query_destroy(machine_t *machine){ // {{{
	query_userdata       *userdata          = (query_userdata *)machine->userdata;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(userdata->data, &r_free);
	
	free(userdata);
	return 0;
} // }}}
static int query_configure(machine_t *machine, hash_t *config){ // {{{
	query_userdata        *userdata          = (query_userdata *)machine->userdata;
	
	if( (userdata->data = hash_data_find(config, HK(data))) == NULL)
		return error("data not supplied");
	
	return 0;
} // }}}

static ssize_t query_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	query_userdata        *userdata          = (query_userdata *)machine->userdata;
	
	ret = data_hash_query(userdata->data, request);
	
	request_t r_next[] = {
		{ HK(ret), DATA_PTR_SIZET(&ret) },
		hash_inline(request),
		hash_end
	};
	return machine_pass(machine, r_next);
} // }}}

machine_t query_proto = {
	.class          = "data/query",
	.supported_api  = API_HASH,
	.func_init      = &query_init,
	.func_configure = &query_configure,
	.func_destroy   = &query_destroy,
	.machine_type_hash = {
		.func_handler = &query_handler
	}
};

