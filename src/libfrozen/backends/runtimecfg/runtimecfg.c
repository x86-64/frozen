#include <libfrozen.h>

#define EMODULE 19

static int runtimecfg_configure_any(backend_t *backend, config_t *config, config_t *forkreq){ // {{{
	ssize_t                ret;
	backend_t             *new_backend;
	config_t              *new_backend_cfg   = NULL;
	
	hash_data_copy(ret, TYPE_HASHT, new_backend_cfg, config, HK(config));
	if(ret != 0)
		return error("backend_config not supplied");
	
	// EDIT config
	
	new_backend = backend_new(new_backend_cfg);
	backend_connect(backend, new_backend); // on destroy - no need to disconnect or call _destory, core will automatically do it
	
	return 0;
} // }}}
static int runtimecfg_configure(backend_t *backend, config_t *config){ // {{{
	return runtimecfg_configure_any(backend, config, NULL);
} // }}}
static int runtimecfg_fork(backend_t *backend, backend_t *parent, config_t *forkreq){ // {{{
	return runtimecfg_configure_any(backend, backend->config, forkreq);
} // }}}

backend_t runtimecfg_proto = {
	.class          = "runtimecfg",
	.supported_api  = API_HASH,
	.func_configure = &runtimecfg_configure,
	.func_fork      = &runtimecfg_fork
};

