#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_kill daemon/kill
 */
/**
 * @ingroup mod_backend_kill
 * @page page_kill_info Description
 *
 * This backend terminate daemon using SIG_TERM. Frozen catch this signal and shutdown properly.
 */
/**
 * @ingroup mod_backend_kill
 * @page page_kill_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "daemon/kill"
 * }
 * @killcode
 */

static ssize_t kill_handler(backend_t *backend, request_t *request){ // {{{
	kill(getpid(), SIGTERM);
	return 0;
} // }}}
static ssize_t kill_fast_handler(backend_t *backend, void *hargs){ // {{{
	kill(getpid(), SIGTERM);
	return 0;
} // }}}

backend_t kill_proto = {
	.class          = "daemon/kill",
	.supported_api  = API_HASH | API_FAST,
	.backend_type_hash = {
		.func_handler = &kill_handler
	},
	.backend_type_fast = {
		.func_handler = &kill_fast_handler
	}
};

