#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_emitter request/emitter
 */
/**
 * @ingroup mod_backend_emitter
 * @page page_emitter_info Description
 *
 * This backend wait for input request and send user defined queries in any amounts. Input request passed to underlying backend,
 * so this could be used to construct sequence of requests.
 */
/**
 * @ingroup mod_backend_emitter
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

#define EMODULE 40
#define N_REQUESTS_DEFAULT 1

typedef struct emitter_userdata {
	emitter_t             *req;
	uintmax_t              nreq;
} emitter_userdata;

static int emitter_init(backend_t *backend){ // {{{
	emitter_userdata    *userdata          = backend->userdata = calloc(1, sizeof(emitter_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->nreq = N_REQUESTS_DEFAULT;
	return 0;
} // }}}
static int emitter_destroy(backend_t *backend){ // {{{
	emitter_userdata      *userdata          = (emitter_userdata *)backend->userdata;
	
	data_t           d_emitter = DATA_PTR_EMITTERT(userdata->req);
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&d_emitter, &r_free);
	
	free(userdata);
	return 0;
} // }}}
static int emitter_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	emitter_userdata      *userdata          = (emitter_userdata *)backend->userdata;
	
	hash_data_convert (ret, TYPE_EMITTERT, userdata->req,       config, HK(request));
	hash_data_copy    (ret, TYPE_UINTT,    userdata->nreq,      config, HK(count));
	return 0;
} // }}}

static ssize_t emitter_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t              ret;
	uintmax_t            i;
	emitter_userdata    *userdata          = (emitter_userdata *)((backend_t *)backend)->userdata;
	
	if(userdata->req){
		data_t           d_emitter = DATA_PTR_EMITTERT(userdata->req);
		fastcall_execute r_exec    = { { 2, ACTION_EXECUTE } };
		
		for(i = 0; i < userdata->nreq; i++)
			data_query(&d_emitter, &r_exec);
	}
	return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
} // }}}

backend_t emitter_proto = {
	.class          = "request/emitter",
	.supported_api  = API_HASH,
	.func_init      = &emitter_init,
	.func_configure = &emitter_configure,
	.func_destroy   = &emitter_destroy,
	.backend_type_hash = {
		.func_handler = &emitter_handler
	}
};

