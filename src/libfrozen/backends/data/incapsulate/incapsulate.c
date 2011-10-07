#include <libfrozen.h>
#define EMODULE 6

typedef struct incap_userdata {
	hash_key_t     key;
	hash_key_t     key_out;
	hash_key_t     key_to;
	hash_key_t     key_from;
	hash_key_t     count;
	hash_key_t     size;
	off_t          multiply;
	data_t         multiply_data;
} incap_userdata;

static int incap_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(incap_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int incap_destroy(backend_t *backend){ // {{{
	incap_userdata *userdata = (incap_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int incap_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	char                  *key_str       = "offset";
	char                  *key_out_str   = "offset_out";
	char                  *key_to_str    = "offset_to";
	char                  *key_from_str  = "offset_from";
	char                  *count_str     = "buffer";
	char                  *size_str      = "size";
	off_t                  multiply      = 1;
	incap_userdata        *userdata      = (incap_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, key_str,       config, HK(key));
	hash_data_copy(ret, TYPE_STRINGT, key_out_str,   config, HK(key_out));
	hash_data_copy(ret, TYPE_STRINGT, key_to_str,    config, HK(key_to));
	hash_data_copy(ret, TYPE_STRINGT, key_from_str,  config, HK(key_from));
	hash_data_copy(ret, TYPE_STRINGT, count_str,     config, HK(count));
	hash_data_copy(ret, TYPE_STRINGT, size_str,      config, HK(size));
	hash_data_copy(ret, TYPE_OFFT,    multiply,      config, HK(multiply));
	
	data_t multiply_data = DATA_PTR_OFFT(&userdata->multiply);
	memcpy(&userdata->multiply_data, &multiply_data, sizeof(multiply_data));
	
	userdata->key      = hash_string_to_key(key_str);
	userdata->key_out  = hash_string_to_key(key_out_str);
	userdata->key_to   = hash_string_to_key(key_to_str);
	userdata->key_from = hash_string_to_key(key_from_str);
	userdata->count    = hash_string_to_key(count_str);
	userdata->size     = hash_string_to_key(size_str);
	userdata->multiply = multiply;
	
	if(multiply == 0)
		return error("backend incapsulate parameter multiply invalid");
	
	return 0;
} // }}}

static ssize_t incap_handler(backend_t *backend, request_t *request){
	ssize_t                ret;
	uintmax_t              size, new_size;
	data_t                *data;
	incap_userdata        *userdata = (incap_userdata *)backend->userdata;

	if( (data = hash_data_find(request, userdata->size)) != NULL){
		data_get(ret, TYPE_UINTT, size, data);
		
		// round size
		new_size =     size / userdata->multiply;
		new_size = new_size * userdata->multiply;
		if( (size % userdata->multiply) != 0)
			new_size += userdata->multiply;
		
		fastcall_write r_write = { { 5, ACTION_WRITE }, 0, &new_size, sizeof(new_size) };
		data_query(data, &r_write);
	}
	
	fastcall_mul r_mul = { { 3, ACTION_MULTIPLY }, &userdata->multiply_data };
	
	if( (data = hash_data_find(request, userdata->key)) != NULL)
		data_query(data, &r_mul);
	if( (data = hash_data_find(request, userdata->key_to)) != NULL)
		data_query(data, &r_mul);
	if( (data = hash_data_find(request, userdata->key_from)) != NULL)
		data_query(data, &r_mul);
	
	ret = (ret = backend_pass(backend, request) < 0) ? ret : -EEXIST;
	
	fastcall_div r_div = { { 3, ACTION_DIVIDE }, &userdata->multiply_data };
	
	if( (data = hash_data_find(request, userdata->key_out)) != NULL)
		data_query(data, &r_div);
	if( (data = hash_data_find(request, userdata->count)) != NULL)
		data_query(data, &r_div);
	
	return ret;
}

backend_t incapsulate_proto = {
	.class          = "data/incapsulate",
	.supported_api  = API_HASH,
	.func_init      = &incap_init,
	.func_configure = &incap_configure,
	.func_destroy   = &incap_destroy,
	.backend_type_hash = {
		.func_handler = &incap_handler,
	}
};

