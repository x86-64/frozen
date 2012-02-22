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
 *              request                 = { ... }                # request for data query, optional, default is incoming request
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID 52
#define ERRORS_MODULE_NAME "data/query"

typedef struct query_userdata {
	data_t                 data;
	request_t             *request;
} query_userdata;

static ssize_t query_init(machine_t *machine){ // {{{
	query_userdata       *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(query_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static ssize_t query_destroy(machine_t *machine){ // {{{
	query_userdata       *userdata          = (query_userdata *)machine->userdata;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&userdata->data, &r_free);
	
	hash_free(userdata->request);
	free(userdata);
	return 0;
} // }}}
static ssize_t query_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	query_userdata        *userdata          = (query_userdata *)machine->userdata;
	
	hash_data_consume(ret, TYPE_HASHT, userdata->request, config, HK(request));
	hash_holder_consume(ret, userdata->data, config, HK(data));
	if(ret != 0)
		return error("data not supplied");
	
	return 0;
} // }}}

static ssize_t query_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	query_userdata        *userdata          = (query_userdata *)machine->userdata;
	
	request_set_current(request);
	
	if( (ret = data_hash_query(
		&userdata->data,
		userdata->request ?
			userdata->request :
			request
	)) < 0)
		return ret;
	
	return machine_pass(machine, request);
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

