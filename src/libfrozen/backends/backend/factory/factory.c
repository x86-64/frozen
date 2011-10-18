#include <libfrozen.h>

#define EMODULE 33

typedef struct factory_userdata {
	config_t              *backend_config;
	hash_key_t             output;
} factory_userdata;

static int factory_init(backend_t *backend){ // {{{
	factory_userdata      *userdata;
	if((userdata = backend->userdata = calloc(1, sizeof(factory_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->output = HK(backend);
	return 0;
} // }}}
static int factory_destroy(backend_t *backend){ // {{{
	factory_userdata      *userdata = (factory_userdata *)backend->userdata;
	free(userdata);
	return 0;
} // }}}
static int factory_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	factory_userdata      *userdata          = (factory_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT,  userdata->output,         config, HK(output));
	hash_data_copy(ret, TYPE_HASHT,     userdata->backend_config, config, HK(config));
	if(ret != 0)
		return error("HK(config) not supplied");
	
	return 0;
} // }}}

static ssize_t factory_handler(backend_t *backend, request_t *request){ // {{{
	data_t                *output;
	backend_t             *child;
	factory_userdata      *userdata = (factory_userdata *)backend->userdata;
	
	if( (output = hash_data_find(request, userdata->output)) == NULL)
		return -EINVAL;
	
	if( output->type != TYPE_BACKENDT )
		return -EINVAL;

	config_t  child_config[] = {
		{ 0, DATA_HASHT(
			hash_inline(request),
			hash_inline(userdata->backend_config),
			hash_end
		)},
		hash_end
	};
	child = backend_new(child_config);
	
	if(child == NULL)
		return error("child creation error");

	backend_connect(backend, child); // on destroy - no need to disconnect or call _destory, core will automatically do it
	
	output->ptr = child;
	return 0;
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

