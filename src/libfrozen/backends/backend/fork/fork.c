#include <libfrozen.h>

#define EMODULE 26

typedef struct fork_userdata {
	hash_key_t             hk_backend;
	config_t              *backend_config;
} fork_userdata;

static int fork_init(backend_t *backend){ // {{{
	fork_userdata         *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(fork_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->hk_backend = HK(backend);
	return 0;
} // }}}
static int fork_destroy(backend_t *backend){ // {{{
	fork_userdata      *userdata = (fork_userdata *)backend->userdata;
	
	if(userdata->backend_config != NULL)
		hash_free(userdata->backend_config);

	free(userdata);
	return 0;
} // }}}
static int fork_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	config_t              *backend_config    = NULL;
	fork_userdata         *userdata          = (fork_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->hk_backend, config, HK(input));
	hash_data_copy(ret, TYPE_HASHT,    backend_config, config, HK(config));
	if(ret != 0)
		return error("HK(config) not supplied");
	
	if( (userdata->backend_config = hash_copy(backend_config)) == NULL)
		return error("not enough memory");
	
	return 0;
} // }}}

static ssize_t fork_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	backend_t             *call_to           = NULL;
	fork_userdata         *userdata          = (fork_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_BACKENDT, call_to, request, userdata->hk_backend);
	if(ret != 0 || call_to == NULL){
		if( (call_to = backend_new(userdata->backend_config)) == NULL)
			return error("child creation error");
		
		request_t r_next[] = { // TODO write call_to to hash, not only pass
			{ userdata->hk_backend, DATA_BACKENDT(call_to) },
			hash_next(request)
		};
		return ( (ret = backend_pass(backend, r_next)) < 0 ) ? ret : -EEXIST;
	}
	
	return ( (ret = backend_pass(backend, request)) < 0 ) ? ret : -EEXIST;
} // }}}

backend_t fork_proto = {
	.class          = "backend/fork",
	.supported_api  = API_HASH,
	.func_init      = &fork_init,
	.func_destroy   = &fork_destroy,
	.func_configure = &fork_configure,
	.backend_type_hash = {
		.func_handler = &fork_handler
	}
};

