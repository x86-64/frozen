#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_kill daemon/kill
 */
/**
 * @ingroup mod_machine_kill
 * @page page_kill_info Description
 *
 * This machine terminate daemon using SIG_TERM. Frozen catch this signal and shutdown properly.
 */
/**
 * @ingroup mod_machine_kill
 * @page page_kill_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "daemon/kill"
 * }
 * @killcode
 */

static ssize_t kill_handler(machine_t *machine, request_t *request){ // {{{
	kill(getpid(), SIGTERM);
	return 0;
} // }}}

machine_t kill_proto = {
	.class          = "daemon/kill",
	.supported_api  = API_HASH,
	.machine_type_hash = {
		.func_handler = &kill_handler
	}
};

