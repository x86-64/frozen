#include <libfrozen.h>

typedef struct incap_userdata {
	hash_key_t     key;
	hash_key_t     key_out;
	hash_key_t     key_to;
	hash_key_t     key_from;
	hash_key_t     count;
	DT_OFFT        multiply;
	data_t         multiply_data;
} incap_userdata;

static int incap_init(chain_t *chain){ // {{{
	if((chain->userdata = calloc(1, sizeof(incap_userdata))) == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
static int incap_destroy(chain_t *chain){ // {{{
	incap_userdata *userdata = (incap_userdata *)chain->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int incap_configure(chain_t *chain, hash_t *config){ // {{{
	ssize_t          ret;
	DT_STRING        key_str       = "offset";
	DT_STRING        key_out_str   = "offset_out";
	DT_STRING        key_to_str    = "offset_to";
	DT_STRING        key_from_str  = "offset_from";
	DT_STRING        count_str     = "buffer";
	DT_OFFT          multiply      = 1;
	incap_userdata  *userdata      = (incap_userdata *)chain->userdata;
	
	hash_copy_data(ret, TYPE_STRING, key_str,       config, HK(key));
	hash_copy_data(ret, TYPE_STRING, key_out_str,   config, HK(key_out));
	hash_copy_data(ret, TYPE_STRING, key_to_str,    config, HK(key_to));
	hash_copy_data(ret, TYPE_STRING, key_from_str,  config, HK(key_from));
	hash_copy_data(ret, TYPE_STRING, count_str,     config, HK(count));
	hash_copy_data(ret, TYPE_OFFT,   multiply,      config, HK(multiply));
	
	data_t multiply_data = DATA_PTR_OFFT(&userdata->multiply);
	memcpy(&userdata->multiply_data, &multiply_data, sizeof(multiply_data));
	
	userdata->key      = hash_string_to_key(key_str);
	userdata->key_out  = hash_string_to_key(key_out_str);
	userdata->key_to   = hash_string_to_key(key_to_str);
	userdata->key_from = hash_string_to_key(key_from_str);
	userdata->count    = hash_string_to_key(count_str);
	userdata->multiply = multiply;
	
	if(userdata->multiply == 0)
		return_error(-EINVAL, "chain 'incapsulate' parameter 'multiply' invalid\n");
	
	return 0;
} // }}}

static ssize_t incap_backend_createwrite(chain_t *chain, request_t *request){
	ssize_t          ret;
	data_t          *offset_data;
	data_ctx_t      *offset_ctx;
	incap_userdata  *userdata = (incap_userdata *)chain->userdata;
	
	if( (offset_data = hash_get_data(request, userdata->key)) != NULL){
		offset_ctx  = hash_get_data_ctx(request, userdata->key);
		
		data_arithmetic('*', offset_data, offset_ctx, &userdata->multiply_data, NULL);
	}
	
	ret = chain_next_query(chain, request);
	
	if( (offset_data = hash_get_data(request, userdata->key_out)) != NULL){
		offset_ctx  = hash_get_data_ctx(request, userdata->key_out);
		
		data_arithmetic('/', offset_data, offset_ctx, &userdata->multiply_data, NULL);
	}
	return ret;
}

static ssize_t incap_backend_read(chain_t *chain, request_t *request){
	data_t          *offset_data;
	data_ctx_t      *offset_ctx;
	incap_userdata  *userdata = (incap_userdata *)chain->userdata;
	
	if( (offset_data = hash_get_data(request, userdata->key)) != NULL){
		offset_ctx  = hash_get_data_ctx(request, userdata->key);
		
		data_arithmetic('*', offset_data, offset_ctx, &userdata->multiply_data, NULL);
	}
	
	return chain_next_query(chain, request);
}

static ssize_t incap_backend_move(chain_t *chain, request_t *request){
	data_t          *offset_data;
	data_ctx_t      *offset_ctx;
	incap_userdata  *userdata = (incap_userdata *)chain->userdata;
	
	if( (offset_data = hash_get_data(request, userdata->key_to)) != NULL){
		offset_ctx  = hash_get_data_ctx(request, userdata->key_to);
		
		data_arithmetic('*', offset_data, offset_ctx, &userdata->multiply_data, NULL);
	}
	if( (offset_data = hash_get_data(request, userdata->key_from)) != NULL){
		offset_ctx  = hash_get_data_ctx(request, userdata->key_from);
		
		data_arithmetic('*', offset_data, offset_ctx, &userdata->multiply_data, NULL);
	}
	
	return chain_next_query(chain, request);
}

static ssize_t incap_backend_count(chain_t *chain, request_t *request){
	ssize_t          ret;
	data_t          *offset_data;
	data_ctx_t      *offset_ctx;
	incap_userdata  *userdata = (incap_userdata *)chain->userdata;
	
	ret = chain_next_query(chain, request);
	
	if( (offset_data = hash_get_data(request, userdata->count)) != NULL){
		offset_ctx  = hash_get_data_ctx(request, userdata->count);
		
		data_arithmetic('/', offset_data, offset_ctx, &userdata->multiply_data, NULL);
	}
	return ret;
}

static chain_t chain_incap = {
	"incapsulate",
	CHAIN_TYPE_CRWD,
	.func_init      = &incap_init,
	.func_configure = &incap_configure,
	.func_destroy   = &incap_destroy,
	{{
		.func_create = &incap_backend_createwrite,
		.func_set    = &incap_backend_createwrite,
		.func_get    = &incap_backend_read,
		.func_delete = &incap_backend_read,
		.func_move   = &incap_backend_move,
		.func_count  = &incap_backend_count
	}}
};
CHAIN_REGISTER(chain_incap)

