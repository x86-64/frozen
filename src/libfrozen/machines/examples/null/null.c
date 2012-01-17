#include <libfrozen.h>

/* this module must be used only as example */

static ssize_t null_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t       ret;
	size_t        value;
	
	hash_data_copy(ret, TYPE_SIZET, value, request, HK(size));
	if(ret == 0 && value == 0x0000BEEF)
		return value;
	
	return machine_pass(machine, request);
} // }}}

machine_t null_proto = {
	.class          = "examples/null",
	.supported_api  = API_HASH,
	.machine_type_hash = {
		.func_handler = &null_handler
	}
};
