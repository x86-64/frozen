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
	uintmax_t              fatal             = 0;
	char                  *input_str         = NULL;
	char                  *output_str        = NULL;
	jenkins_userdata       *userdata          = (jenkins_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, input_str,      config, HK(input));
	hash_data_copy(ret, TYPE_STRINGT, output_str,     config, HK(output));
	hash_data_copy(ret, TYPE_UINTT,   fatal,          config, HK(fatal));
	
	userdata->input   = hash_string_to_key(input_str);
	userdata->output  = hash_string_to_key(output_str);
	userdata->fatal   = ( fatal == 0 ) ? 0 : 1;
	return 0;
} // }}}

static ssize_t jenkins_32_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uint32_t               hash              = 0;
	data_t                *key               = NULL;
	jenkins_userdata      *userdata          = (jenkins_userdata *)backend->userdata;
	
	hash_data_find(request, userdata->input, &key, NULL);
	if(key == NULL){
		if(userdata->fatal == 0)
			return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
		return error("input key not supplied");
	}
	
	jenkins32_hash(0, data_value_ptr(key), data_value_len(key), (uint32_t *)&hash, 1); // TODO remove data_value_*
	
	request_t r_next[] = {
		{ userdata->output, DATA_PTR_UINT32T(&hash) },
		hash_next(request)
	};
	return ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
} // }}}

static ssize_t jenkins_64_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uint64_t               hash              = 0;
	data_t                *key               = NULL;
	jenkins_userdata      *userdata          = (jenkins_userdata *)backend->userdata;
	
	hash_data_find(request, userdata->input, &key, NULL);
	if(key == NULL){
		if(userdata->fatal == 0)
			return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
		return error("input key not supplied");
	}
	
	jenkins64_hash(0, data_value_ptr(key), data_value_len(key), (uint64_t *)&hash, 1); // TODO remove data_value_*
	
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

