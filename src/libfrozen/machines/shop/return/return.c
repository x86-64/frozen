#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_return shop/return
 */
/**
 * @ingroup mod_machine_return
 * @page page_return_info Description
 *
 * This machine return current request to user-supplied machine.
 */
/**
 * @ingroup mod_machine_return
 * @page page_return_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "shop/return",
 *              return_to               = (hashkey_t)"return_to"       # return_to hash key, default "return_to"
 * }
 * @endcode
 */

#define EMODULE 26

typedef struct return_userdata {
	hashkey_t              return_to;
} return_userdata;

static int return_init(machine_t *machine){ // {{{
	return_userdata       *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(return_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->return_to = HK(return_to);
	return 0;
} // }}}
static int return_destroy(machine_t *machine){ // {{{
	return_userdata       *userdata = (return_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int return_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	return_userdata       *userdata          = (return_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->return_to, config, HK(return_to));
	
	return 0;
} // }}}

static ssize_t return_handler(machine_t *machine, request_t *request){ // {{{
	hash_t                  *return_hash;
	return_userdata         *userdata          = (return_userdata *)machine->userdata;
	
	if( (return_hash = hash_find(request, userdata->return_to)) == NULL)
		return -EINVAL;
	
	return_hash->key = 0; // remove this hash item from hash, so other return_to keys would be visible
	
	fastcall_query r_query = { { 3, ACTION_QUERY }, request };
	return data_query(&return_hash->data, &r_query);
} // }}}

machine_t return_proto = {
	.class          = "shop/return",
	.supported_api  = API_HASH,
	.func_init      = &return_init,
	.func_destroy   = &return_destroy,
	.func_configure = &return_configure,
	.machine_type_hash = {
		.func_handler = &return_handler
	}
};

