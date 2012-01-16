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
 * }
 * @endcode
 */

#define EMODULE 25

typedef struct pass_userdata {
	data_t                *machine;
} pass_userdata;

static int pass_init(machine_t *machine){ // {{{
	pass_userdata         *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(pass_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int pass_destroy(machine_t *machine){ // {{{
	pass_userdata      *userdata = (pass_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int pass_configure(machine_t *machine, config_t *config){ // {{{
	pass_userdata         *userdata          = (pass_userdata *)machine->userdata;
	
	return ( (userdata->machine = hash_data_find(config, HK(shop))) == NULL) ?
		-EINVAL : 0;
} // }}}

static ssize_t pass_handler(machine_t *machine, request_t *request){ // {{{
	pass_userdata         *userdata          = (pass_userdata *)machine->userdata;
	
	request_t r_next[] = {
		//{ HK(return_to), 
		hash_inline(request),
		hash_end
	};
	
	fastcall_query r_query = { { 3, ACTION_QUERY }, r_next };
	return data_query(userdata->machine, &r_query);
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

