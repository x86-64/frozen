#include <libfrozen.h>
#include <hash_jenkins.h>

#define EMODULE 23

typedef struct jenkins_userdata {
	hash_key_t             input;
	hash_key_t             output;
	uintmax_t              fatal;
} jenkins_userdata;

static int jenkins_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(jenkins_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int jenkins_destroy(backend_t *backend){ // {{{
	jenkins_userdata       *userdata = (jenkins_userdata *)backend->userdata;

	free(userdata);
	return 0;
} // }}}
static int jenkins_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	jenkins_userdata      *userdata          = (jenkins_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->input,      config, HK(input));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->output,     config, HK(output));
	hash_data_copy(ret, TYPE_UINTT,    userdata->fatal,      config, HK(fatal));
	
	userdata->fatal   = ( userdata->fatal == 0 ) ? 0 : 1;
	return 0;
} // }}}

static ssize_t jenkins_32_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uint32_t               hash;
	data_t                *key               = NULL;
	jenkins_userdata      *userdata          = (jenkins_userdata *)backend->userdata;
	
	key = hash_data_find(request, userdata->input);
	if(key == NULL){
		if(userdata->fatal == 0)
			return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
		return error("input key not supplied");
	}
	
	hash = jenkins32_hash(0, key);
	
	request_t r_next[] = {
		{ userdata->output, DATA_PTR_UINT32T(&hash) },
		hash_next(request)
	};
	return ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
} // }}}

static ssize_t jenkins_64_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uint64_t               hash;
	data_t                *key               = NULL;
	jenkins_userdata      *userdata          = (jenkins_userdata *)backend->userdata;
	
	key = hash_data_find(request, userdata->input);
	if(key == NULL){
		if(userdata->fatal == 0)
			return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
		return error("input key not supplied");
	}

	hash = jenkins64_hash(0, key);
	
	request_t r_next[] = {
		{ userdata->output, DATA_PTR_UINT64T(&hash) },
		hash_next(request)
	};
	return ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
} // }}}

backend_t jenkins32_proto = {
	.class          = "hash/jenkins32",
	.supported_api  = API_HASH,
	.func_init      = &jenkins_init,
	.func_destroy   = &jenkins_destroy,
	.func_configure = &jenkins_configure,
	.backend_type_hash = {
		.func_handler  = &jenkins_32_handler
	}
};

backend_t jenkins64_proto = {
	.class          = "hash/jenkins64",
	.supported_api  = API_HASH,
	.func_init      = &jenkins_init,
	.func_destroy   = &jenkins_destroy,
	.func_configure = &jenkins_configure,
	.backend_type_hash = {
		.func_handler  = &jenkins_64_handler
	}
};

