#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_debug request/debug
 */
/**
 * @ingroup mod_machine_debug
 * @page page_debug_info Description
 *
 * This machine prints to stdout request passed to it in human readable form
 */
/**
 * @ingroup mod_machine_debug
 * @page page_debug_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "request/debug",
 *              after                   = (uint_t)'0',        # print request after passing to underlying machine, default 0
 *              before                  = (uint_t)'1',        # print request before passing to underlying machine, default 1
 *              verbose                 = (uint_t)'1',        # show hash content, default 1
 * }
 * @endcode
 */

#define EMODULE 16

typedef enum debug_flags {
	DEBUG_BEFORE = 1,
	DEBUG_AFTER  = 2
} debug_flags;

typedef struct debug_userdata {
	debug_flags            flags;
	uintmax_t              show_dump;
} debug_userdata;

static void debug_do(machine_t *machine, request_t *request, debug_flags flag){ // {{{
	debug_userdata        *userdata          = (debug_userdata *)machine->userdata;
	
	printf("debug %s: machine: %p, request: %p, machine->name: %s\n",
		( flag == DEBUG_BEFORE ) ? "before" : "after",
		machine,
		request,
		machine->name
	);
	
	if(userdata->show_dump != 0){
		hash_dump(request);
	}
} // }}}

static int debug_init(machine_t *machine){ // {{{
	debug_userdata        *userdata          = machine->userdata = calloc(1, sizeof(debug_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int debug_destroy(machine_t *machine){ // {{{
	debug_userdata        *userdata          = (debug_userdata *)machine->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int debug_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              cfg_showdump      = 1;
	uintmax_t              cfg_before        = 1;
	uintmax_t              cfg_after         = 0;
	debug_userdata        *userdata          = (debug_userdata *)machine->userdata;
	
	hash_data_copy(ret, TYPE_UINTT, cfg_showdump, config, HK(verbose));
	hash_data_copy(ret, TYPE_UINTT, cfg_before,   config, HK(before));
	hash_data_copy(ret, TYPE_UINTT, cfg_after,    config, HK(after));
	
	userdata->flags |= (cfg_before != 0) ? DEBUG_BEFORE : 0;
	userdata->flags |= (cfg_after  != 0) ? DEBUG_AFTER  : 0;
	userdata->show_dump = cfg_showdump;
	
	return 0;
} // }}}

static ssize_t debug_request(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	debug_userdata        *userdata          = (debug_userdata *)machine->userdata;
	
	if( (userdata->flags & DEBUG_BEFORE) != 0)
		debug_do(machine, request, DEBUG_BEFORE);
	
	ret = ( (ret = machine_pass(machine, request)) < 0) ? ret : -EEXIST;
	
	if( (userdata->flags & DEBUG_AFTER) != 0)
		debug_do(machine, request, DEBUG_AFTER);
	
	return ret;
} // }}}

machine_t debug_proto = {
	.class          = "request/debug",
	.supported_api  = API_HASH,
	.func_init      = &debug_init,
	.func_configure = &debug_configure,
	.func_destroy   = &debug_destroy,
	.machine_type_hash = {
		.func_handler = &debug_request,
	}
};


