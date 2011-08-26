#include <libfrozen.h>

#define EMODULE 16

typedef enum debug_flags {
	DEBUG_BEFORE = 1,
	DEBUG_AFTER  = 2
} debug_flags;

typedef struct debug_userdata {
	debug_flags            flags;
	uintmax_t              show_dump;
} debug_userdata;

static void debug_do(backend_t *backend, request_t *request, debug_flags flag){ // {{{
	debug_userdata        *userdata          = (debug_userdata *)backend->userdata;
	
	printf("debug %s: backend: %p, request: %p, backend->name: %s\n",
		( flag == DEBUG_BEFORE ) ? "before" : "after",
		backend,
		request,
		backend_get_name(backend)
	);
	
#ifdef DEBUG
	if(userdata->show_dump != 0){
		hash_dump(request);
	}
#endif
} // }}}

static int debug_init(backend_t *backend){ // {{{
	debug_userdata        *userdata          = backend->userdata = calloc(1, sizeof(debug_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int debug_destroy(backend_t *backend){ // {{{
	debug_userdata        *userdata          = (debug_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int debug_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              cfg_showdump      = 1;
	uintmax_t              cfg_before        = 1;
	uintmax_t              cfg_after         = 0;
	debug_userdata        *userdata          = (debug_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_UINTT, cfg_showdump, config, HK(verbose));
	hash_data_copy(ret, TYPE_UINTT, cfg_before,   config, HK(before));
	hash_data_copy(ret, TYPE_UINTT, cfg_after,    config, HK(after));
	
	userdata->flags |= (cfg_before != 0) ? DEBUG_BEFORE : 0;
	userdata->flags |= (cfg_after  != 0) ? DEBUG_AFTER  : 0;
	userdata->show_dump = cfg_showdump;
	
	return 0;
} // }}}

static ssize_t debug_request(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	debug_userdata        *userdata          = (debug_userdata *)backend->userdata;
	
	if( (userdata->flags & DEBUG_BEFORE) != 0)
		debug_do(backend, request, DEBUG_BEFORE);
	
	ret = ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	if( (userdata->flags & DEBUG_AFTER) != 0)
		debug_do(backend, request, DEBUG_AFTER);
	
	return ret;
} // }}}

backend_t debug_proto = {
	.class          = "request/debug",
	.supported_api  = API_HASH,
	.func_init      = &debug_init,
	.func_configure = &debug_configure,
	.func_destroy   = &debug_destroy,
	.backend_type_hash = {
		.func_handler = &debug_request,
	}
};


