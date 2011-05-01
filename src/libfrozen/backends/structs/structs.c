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

	uintmax_t      copy;
} struct_userdata;

static int struct_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(struct_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int struct_destroy(backend_t *backend){ // {{{
	struct_userdata *userdata = (struct_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int struct_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t           ret;
	DT_STRING         key_str       = "buffer";
	DT_STRING         key_value_str = NULL;
	DT_STRING         size_str      = NULL;
	DT_HASHT          struct_hash   = NULL;
	uintmax_t         copy          = 0;
	struct_userdata  *userdata      = (struct_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHT,   struct_hash,   config, HK(structure));
	hash_data_copy(ret, TYPE_STRINGT, key_str,       config, HK(key));
	hash_data_copy(ret, TYPE_STRINGT, key_value_str, config, HK(values));
	hash_data_copy(ret, TYPE_STRINGT, size_str,      config, HK(size));
	hash_data_copy(ret, TYPE_UINTT,   copy,          config, HK(copy));
	
	userdata->key        = hash_string_to_key(key_str);
	userdata->key_values = hash_string_to_key(key_value_str);
	userdata->size       = hash_string_to_key(size_str);
	userdata->structure  = struct_hash;
	userdata->values     = (userdata->key_values == 0) ? STRUCT_VALUES_WHOLE : STRUCT_VALUES_ONE;
	userdata->copy       = copy;
	
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
	data_ctx_t      *buffer_ctx;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)backend->userdata;
	
	hash_data_find(request, userdata->key, &buffer, &buffer_ctx);
	if(buffer != NULL){
		switch(userdata->values){
			case STRUCT_VALUES_WHOLE: values = request; break;
			case STRUCT_VALUES_ONE:
				hash_data_copy(ret, TYPE_HASHT, values, request, userdata->key_values);
				if(ret != 0)
					return warning("hash with keys not supplied");
				break;
		};
		
		if( (struct_size = struct_pack(userdata->structure, values, buffer, buffer_ctx)) == 0)
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
	data_ctx_t      *buffer_ctx;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)backend->userdata;
	
	ret = (ret = backend_pass(backend, request)) < 0 ? ret : -EEXIST;
	
	hash_data_find(request, userdata->key, &buffer, &buffer_ctx);
	if(buffer != NULL){
		switch(userdata->values){
			case STRUCT_VALUES_WHOLE: values = request; break;
			case STRUCT_VALUES_ONE:
				hash_data_copy(ret2, TYPE_HASHT, values, request, userdata->key_values);
				if(ret2 != 0)
					return warning("hash with keys not supplied");
				break;
		};
		
		if(userdata->copy == 0){
			if(struct_unpack(userdata->structure, values, buffer, buffer_ctx) == 0)
				return error("struct_unpack failed");
		}else{
			if(struct_unpack_copy(userdata->structure, values, buffer, buffer_ctx) == 0)
				return error("struct_unpack_copy failed");
		}
	}
	
	return ret;
}

backend_t structs_proto = {
	.class          = "structs",
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


