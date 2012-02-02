#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_try shop/try
 */
/**
 * @ingroup mod_machine_try
 * @page page_try_info Description
 *
 * This machine try pass current request to user-supplied machine. Even if error occured - it pass request with error number to next machine.
 */
/**
 * @ingroup mod_machine_try
 * @page page_try_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "shop/try",
 *              shop                    =                         # try to this shop
 *                                        (env_t)'machine',       #  - to shop supplied in user request under "machine" hashkey 
 *                                        (machine_t)'name',      #  - to shop named "name"
 *                                        ...
 *              return_to               = (hashkey_t)"return_to"  # return_to hash key, default "return_to"
 * }
 * @endcode
 */

#define EMODULE 54

typedef struct try_userdata {
	data_t                 machine;
	hashkey_t              return_to;
} try_userdata;

static int try_init(machine_t *machine){ // {{{
	try_userdata         *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(try_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->return_to = HK(return_to);
	return 0;
} // }}}
static int try_destroy(machine_t *machine){ // {{{
	try_userdata      *userdata = (try_userdata *)machine->userdata;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&userdata->machine, &r_free);
	
	free(userdata);
	return 0;
} // }}}
static int try_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	try_userdata         *userdata          = (try_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->return_to, config, HK(return_to));
	hash_holder_consume(ret, userdata->machine, config, HK(shop));
	if(ret != 0)
		return error("shop parameter not supplied");
	
	return 0;
} // }}}

static ssize_t try_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t               ret;
	try_userdata         *userdata          = (try_userdata *)machine->userdata;
	
	request_t r_next[] = {
		{ userdata->return_to, DATA_NEXT_MACHINET(machine) },
		hash_inline(request),
		hash_end
	};
	
	fastcall_query r_query = { { 3, ACTION_QUERY }, r_next };
	ret = data_query(&userdata->machine, &r_query);
	
	request_t r_pass[] = {
		{ HK(ret), DATA_PTR_SIZET(&ret) },
		hash_inline(request),
		hash_end
	};
	return machine_pass(machine, r_pass);
} // }}}

machine_t try_proto = {
	.class          = "shop/try",
	.supported_api  = API_HASH,
	.func_init      = &try_init,
	.func_destroy   = &try_destroy,
	.func_configure = &try_configure,
	.machine_type_hash = {
		.func_handler = &try_handler
	}
};

