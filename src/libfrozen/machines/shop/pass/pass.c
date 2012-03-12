#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_pass shop/pass
 */
/**
 * @ingroup mod_machine_pass
 * @page page_pass_info Description
 *
 * This machine pass current request to user-supplied machine.
 */
/**
 * @ingroup mod_machine_pass
 * @page page_pass_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "shop/pass",
 *              shop                    =                         # pass to this shop
 *                                        (env_t)'machine',       #  - to shop supplied in user request under "machine" hashkey 
 *                                        (machine_t)'name',      #  - to shop named "name"
 *                                        ...
 *              return_to               = (hashkey_t)"return_to"  # return_to hash key, default "return_to"
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID 25
#define ERRORS_MODULE_NAME "shop/pass"

typedef struct pass_userdata {
	data_t                 machine;
	hashkey_t              return_to;
} pass_userdata;

static ssize_t pass_init(machine_t *machine){ // {{{
	pass_userdata         *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(pass_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->return_to = HK(return_to);
	return 0;
} // }}}
static ssize_t pass_destroy(machine_t *machine){ // {{{
	pass_userdata      *userdata = (pass_userdata *)machine->userdata;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&userdata->machine, &r_free);
	
	free(userdata);
	return 0;
} // }}}
static ssize_t pass_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	pass_userdata         *userdata          = (pass_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->return_to, config, HK(return_to));
	hash_holder_consume(ret, userdata->machine, config, HK(shop));
	if(ret != 0)
		return error("shop parameter not supplied");
	
	return 0;
} // }}}

static ssize_t pass_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	pass_userdata         *userdata          = (pass_userdata *)machine->userdata;
	
	request_enter_context(request);
	
		request_t r_next[] = {
			{ userdata->return_to, DATA_NEXT_MACHINET(machine) },
			hash_inline(request),
			hash_end
		};
		
		fastcall_query r_query = { { 3, ACTION_QUERY }, r_next };
		ret = data_query(&userdata->machine, &r_query);

	request_leave_context();
	return ret;
} // }}}

machine_t pass_proto = {
	.class          = "shop/pass",
	.supported_api  = API_HASH,
	.func_init      = &pass_init,
	.func_destroy   = &pass_destroy,
	.func_configure = &pass_configure,
	.machine_type_hash = {
		.func_handler = &pass_handler
	}
};

