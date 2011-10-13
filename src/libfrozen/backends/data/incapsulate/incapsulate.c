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
	incap_userdata        *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(incap_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->key      = HK(offset);
	userdata->key_out  = HK(offset_out);
	userdata->key_to   = HK(offset_to);
	userdata->key_from = HK(offset_from);
	userdata->count    = HK(buffer);
	userdata->size     = HK(size);
	userdata->multiply = 1;
	
	data_t multiply_data = DATA_PTR_OFFT(&userdata->multiply);
	memcpy(&userdata->multiply_data, &multiply_data, sizeof(multiply_data));
	
	return 0;
} // }}}
static int incap_destroy(backend_t *backend){ // {{{
	incap_userdata *userdata = (incap_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int incap_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	incap_userdata        *userdata      = (incap_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->key,       config, HK(key));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->key_out,   config, HK(key_out));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->key_to,    config, HK(key_to));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->key_from,  config, HK(key_from));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->count,     config, HK(count));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->size,      config, HK(size));
	hash_data_copy(ret, TYPE_OFFT,     userdata->multiply,  config, HK(multiply));
	
	if(userdata->multiply == 0)
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

