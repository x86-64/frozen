#include <libfrozen.h>
#define EMODULE 5

typedef enum struct_values {
	STRUCT_VALUES_WHOLE,
	STRUCT_VALUES_ONE
} struct_values;

typedef struct struct_userdata {
	struct_t      *structure;
	hash_key_t     key;
	hash_key_t     key_values;
	hash_key_t     size;
	struct_values  values;
} struct_userdata;

static int struct_init(backend_t *backend){ // {{{
	struct_userdata       *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(struct_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->key = HK(buffer);
	
	return 0;
} // }}}
static int struct_destroy(backend_t *backend){ // {{{
	struct_userdata *userdata = (struct_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int struct_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	hash_t                *struct_hash   = NULL;
	struct_userdata       *userdata      = (struct_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHT,    struct_hash,          config, HK(structure));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->key,        config, HK(key));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->key_values, config, HK(values));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->size,       config, HK(size));
	
	userdata->structure  = struct_hash;
	userdata->values     = (userdata->key_values == 0) ? STRUCT_VALUES_WHOLE : STRUCT_VALUES_ONE;
	
	if(userdata->key == 0)
		return error("backend struct parameter key invalid");
	if(userdata->structure == NULL)
		return error("backend struct parameter structure invalid");
	
	return 0;
} // }}}

static ssize_t struct_backend_pack(backend_t *backend, request_t *request){
	ssize_t          ret;
	size_t           struct_size;
	data_t          *buffer;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)backend->userdata;
	
	buffer = hash_data_find(request, userdata->key);
	if(buffer != NULL){
		switch(userdata->values){
			case STRUCT_VALUES_WHOLE: values = request; break;
			case STRUCT_VALUES_ONE:
				hash_data_copy(ret, TYPE_HASHT, values, request, userdata->key_values);
				if(ret != 0)
					return warning("hash with keys not supplied");
				break;
		};
		
		if( (struct_size = struct_pack(userdata->structure, values, buffer)) == 0)
			return error("struct_pack failed");
		
		request_t new_request[] = {
			{ userdata->size, DATA_PTR_SIZET(&struct_size) },
			hash_next(request)
		};
		
		return ( (ret = backend_pass(backend, new_request)) < 0) ? ret : -EEXIST;
	}
	
	return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
}

static ssize_t struct_backend_unpack(backend_t *backend, request_t *request){
	ssize_t          ret, ret2;
	data_t          *buffer;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)backend->userdata;
	
	ret = (ret = backend_pass(backend, request)) < 0 ? ret : -EEXIST;
	
	buffer = hash_data_find(request, userdata->key);
	if(buffer != NULL){
		switch(userdata->values){
			case STRUCT_VALUES_WHOLE: values = request; break;
			case STRUCT_VALUES_ONE:
				hash_data_copy(ret2, TYPE_HASHT, values, request, userdata->key_values);
				if(ret2 != 0)
					return warning("hash with keys not supplied");
				break;
		};
		
		if(struct_unpack(userdata->structure, values, buffer) == 0)
			return error("struct_unpack failed");
	}
	
	return ret;
}

backend_t structs_proto = {
	.class          = "data/structs",
	.supported_api  = API_CRWD,
	.func_init      = &struct_init,
	.func_configure = &struct_configure,
	.func_destroy   = &struct_destroy,
	{
		.func_create = &struct_backend_pack,
		.func_set    = &struct_backend_pack,
		.func_get    = &struct_backend_unpack
	}
};


