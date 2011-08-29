#include <libfrozen.h>

#define EMODULE 20

typedef struct factory_userdata {
	uintmax_t              running;
	config_t              *backend_config;
} factory_userdata;

static int factory_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(factory_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int factory_destroy(backend_t *backend){ // {{{
	factory_userdata      *userdata = (factory_userdata *)backend->userdata;
	
	if(userdata->backend_config != NULL)
		hash_free(userdata->backend_config);

	free(userdata);
	return 0;
} // }}}
static int factory_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	config_t              *c_backend_config  = NULL;
	factory_userdata      *userdata          = (factory_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHT, c_backend_config, config, HK(config));
	if(ret != 0)
		return error("HK(config) not supplied");
	
	userdata->running = 0;
	if( (userdata->backend_config = hash_copy(c_backend_config)) == NULL)
		return error("not enough memory");
	
	return 0;
} // }}}

static ssize_t factory_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	backend_t             *child;
	factory_userdata      *userdata = (factory_userdata *)backend->userdata;

	if(userdata->running == 0){
		hash_chain(request, userdata->backend_config);  // request overrides all initial configure values
		
		config_t  child_config[] = {
			{ 0, DATA_PTR_HASHT(request) },
			hash_end
		};

		child = backend_new(child_config);
		hash_unchain(request, userdata->backend_config); // TODO SECURITY need swap it, but it will cause multithreading problems
		
		if(child == NULL)
			return error("child creation error");

		backend_connect(backend, child); // on destroy - no need to disconnect or call _destory, core will automatically do it
		userdata->running = 1;
	}

	return ( (ret = backend_pass(backend, request)) < 0 ) ? ret : -EEXIST;
} // }}}

backend_t factory_proto = {
	.class          = "backend/factory",
	.supported_api  = API_HASH,
	.func_init      = &factory_init,
	.func_destroy   = &factory_destroy,
	.func_configure = &factory_configure,
	.backend_type_hash = {
		.func_handler = &factory_handler
	}
};

