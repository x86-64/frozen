#include <libfrozen.h>

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

static int struct_init(chain_t *chain){ // {{{
	if((chain->userdata = calloc(1, sizeof(struct_userdata))) == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
static int struct_destroy(chain_t *chain){ // {{{
	struct_userdata *userdata = (struct_userdata *)chain->userdata;
	
	hash_free(userdata->structure);
	free(userdata);
	return 0;
} // }}}
static int struct_configure(chain_t *chain, hash_t *config){ // {{{
	ssize_t           ret;
	DT_STRING         key_str       = "buffer";
	DT_STRING         key_value_str = NULL;
	DT_STRING         size_str      = NULL;
	DT_HASHT          struct_hash   = NULL;
	struct_userdata  *userdata      = (struct_userdata *)chain->userdata;
	
	hash_copy_data(ret, TYPE_HASHT,  struct_hash,   config, HK(structure));
	hash_copy_data(ret, TYPE_STRING, key_str,       config, HK(key));
	hash_copy_data(ret, TYPE_STRING, key_value_str, config, HK(values));
	hash_copy_data(ret, TYPE_STRING, size_str,      config, HK(size));
	
	userdata->key        = hash_string_to_key(key_str);
	userdata->key_values = hash_string_to_key(key_value_str);
	userdata->size       = hash_string_to_key(size_str);
	userdata->structure  = hash_copy(struct_hash);
	userdata->values     = (userdata->key_values == 0) ? STRUCT_VALUES_WHOLE : STRUCT_VALUES_ONE;
	
	if(userdata->key == 0)
		return_error(-EINVAL, "chain 'struct' parameter 'key' invalid\n");
	if(userdata->structure == NULL)
		return_error(-EINVAL, "chain 'struct' parameter 'structure' invalid\n");
	
	return 0;
} // }}}

static ssize_t struct_backend_pack(chain_t *chain, request_t *request){
	size_t           struct_size;
	data_t          *buffer;
	data_ctx_t      *buffer_ctx;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)chain->userdata;
	
	if( (buffer = hash_get_data(request, userdata->key)) != NULL){
		buffer_ctx = hash_get_data_ctx (request, userdata->key);
		
		switch(userdata->values){
			case STRUCT_VALUES_WHOLE: values = request; break;
			case STRUCT_VALUES_ONE:   values = data_value_ptr(hash_get_data(request, userdata->key_values)); break;
		};
		
		if( (struct_size = struct_pack(userdata->structure, values, buffer, buffer_ctx)) == 0)
			return -EFAULT;
		
		request_t new_request[] = {
			{ userdata->size, DATA_PTR_SIZET(&struct_size) },
			hash_next(request)
		};
		
		return chain_next_query(chain, new_request);
	}
	
	return chain_next_query(chain, request);
}

static ssize_t struct_backend_unpack(chain_t *chain, request_t *request){
	ssize_t          ret;
	data_t          *buffer;
	data_ctx_t      *buffer_ctx;
	request_t       *values;
	struct_userdata *userdata = (struct_userdata *)chain->userdata;
	
	ret = chain_next_query(chain, request);
	
	if( (buffer = hash_get_data(request, userdata->key)) != NULL){
		buffer_ctx = hash_get_data_ctx (request, userdata->key);
		
		switch(userdata->values){
			case STRUCT_VALUES_WHOLE: values = request; break;
			case STRUCT_VALUES_ONE:   values = data_value_ptr(hash_get_data(request, userdata->key_values)); break;
		};
		
		if(struct_unpack(userdata->structure, values, buffer, buffer_ctx) == 0)
			return -EFAULT;
	}
	
	return ret;
}

static chain_t chain_struct = {
	"structs",
	CHAIN_TYPE_CRWD,
	.func_init      = &struct_init,
	.func_configure = &struct_configure,
	.func_destroy   = &struct_destroy,
	{{
		.func_create = &struct_backend_pack,
		.func_set    = &struct_backend_pack,
		.func_get    = &struct_backend_unpack
	}}
};
CHAIN_REGISTER(chain_struct)

