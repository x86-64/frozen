#include <libfrozen.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_emitter request/emitter
 */
/**
 * @ingroup mod_machine_emitter
 * @page page_emitter_info Description
 *
 * This machine wait for input request and send user defined queries in any amounts. Input request passed to underlying machine,
 * so this could be used to construct sequence of requests.
 */
/**
 * @ingroup mod_machine_emitter
 * @page page_emitter_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "request/emitter",
 *              request                 = { ... },            # request to emit (see emitter_t data)
 *              count                   = (uint_t)'1000',     # number of emits, default 1
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID 40
#define ERRORS_MODULE_NAME "request/emitter"
#define N_REQUESTS_DEFAULT 1

typedef struct emitter_userdata {
	emitter_t             *req;
	uintmax_t              nreq;
	uintmax_t              silent;
} emitter_userdata;

static ssize_t emitter_init(machine_t *machine){ // {{{
	emitter_userdata    *userdata          = machine->userdata = calloc(1, sizeof(emitter_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->nreq = N_REQUESTS_DEFAULT;
	return 0;
} // }}}
static ssize_t emitter_destroy(machine_t *machine){ // {{{
	emitter_userdata      *userdata          = (emitter_userdata *)machine->userdata;
	
	data_t           d_emitter = DATA_PTR_EMITTERT(userdata->req);
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&d_emitter, &r_free);
	
	free(userdata);
	return 0;
} // }}}
static ssize_t emitter_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	emitter_userdata      *userdata          = (emitter_userdata *)machine->userdata;
	
	hash_data_convert (ret, TYPE_EMITTERT, userdata->req,       config, HK(request)); if(ret != 0) return ret;
	hash_data_get     (ret, TYPE_UINTT,    userdata->nreq,      config, HK(count));
	hash_data_get     (ret, TYPE_UINTT,    userdata->silent,    config, HK(silent));
	return 0;
} // }}}

static ssize_t emitter_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t              ret;
	uintmax_t            i;
	emitter_userdata    *userdata          = (emitter_userdata *)((machine_t *)machine)->userdata;
	
	if(userdata->req){
		data_t           d_emitter = DATA_PTR_EMITTERT(userdata->req);
		fastcall_execute r_exec    = { { 2, ACTION_EXECUTE } };
		
		for(i = 0; i < userdata->nreq; i++){
			if( (ret = data_query(&d_emitter, &r_exec)) < 0){
				if(userdata->silent == 0)
					errors_log("emitter error: %d: %s\n", ret, errors_describe(ret));
			}
		}
	}
	return machine_pass(machine, request);
} // }}}

machine_t emitter_proto = {
	.class          = "request/emitter",
	.supported_api  = API_HASH,
	.func_init      = &emitter_init,
	.func_configure = &emitter_configure,
	.func_destroy   = &emitter_destroy,
	.machine_type_hash = {
		.func_handler = &emitter_handler
	}
};

