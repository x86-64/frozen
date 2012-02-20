#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_end request/null
 */
/**
 * @ingroup mod_machine_end
 * @page page_end_info Description
 *
 * This machine terminate do nothing, just pas request. Can be useful to make named machines.
 */
/**
 * @ingroup mod_machine_end
 * @page page_end_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "request/null",
 * }
 * @endcode
 */

static ssize_t null_handler(machine_t *machine, request_t *request){ // {{{
	return machine_pass(machine, request);
} // }}}

machine_t null_proto = {
	.class          = "request/null",
	.supported_api  = API_HASH,
	.machine_type_hash = {
		.func_handler = &null_handler
	}
};
