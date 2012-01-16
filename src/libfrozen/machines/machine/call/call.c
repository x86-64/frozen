#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_call machine/call
 */
/**
 * @ingroup mod_machine_call
 * @page page_call_info Description
 *
 * This machine call user-supplied machine with current request. If user give empty machine value, then
 * request passed to underlying machine, and then "call" make one more attempt.
 *
 * Underlying machine is expected to be machine-creating, but this is optional.
 */
/**
 * @ingroup mod_machine_call
 * @page page_call_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "machine/call",
 *              machine                 = (hashkey_t)'name',  # key with machine in request, default "machine"
 *              retry                   = (uint_t)'1',        # rerun request after failed call, default 1
 * }
 * @endcode
 */

#define EMODULE 25

typedef struct call_userdata {
	uintmax_t              retry_request;
	hashkey_t             hk_machine;
} call_userdata;

static int call_init(machine_t *machine){ // {{{
	call_userdata         *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(call_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->hk_machine    = HK(machine);
	userdata->retry_request = 1;
	return 0;
} // }}}
static int call_destroy(machine_t *machine){ // {{{
	call_userdata      *userdata = (call_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int call_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	call_userdata         *userdata          = (call_userdata *)machine->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->hk_machine,    config, HK(input));
	hash_data_copy(ret, TYPE_UINTT,    userdata->retry_request, config, HK(retry));
	return 0;
} // }}}

static ssize_t call_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	machine_t             *call_to           = NULL;
	call_userdata         *userdata          = (call_userdata *)machine->userdata;
	
	hash_data_copy(ret, TYPE_MACHINET, call_to, request, userdata->hk_machine);
	if(call_to)
		return ( (ret = machine_query(call_to, request)) < 0 ) ? ret : -EEXIST;
	
	ret = ( (ret = machine_pass(machine, request)) < 0 ) ? ret : -EEXIST;
	
	if(userdata->retry_request != 0){
		hash_data_copy(ret, TYPE_MACHINET, call_to, request, userdata->hk_machine);
		if(call_to)
			return ( (ret = machine_query(call_to, request)) < 0 ) ? ret : -EEXIST;
	}
	return ret;
} // }}}

machine_t call_proto = {
	.class          = "machine/call",
	.supported_api  = API_HASH,
	.func_init      = &call_init,
	.func_destroy   = &call_destroy,
	.func_configure = &call_configure,
	.machine_type_hash = {
		.func_handler = &call_handler
	}
};

