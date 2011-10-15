#include <libfrozen.h>

#define EMODULE              31

typedef struct hashtable_userdata {
	hash_key_t           input;
	uintmax_t            hashtable_size;
} hashtable_userdata;

static int hashtable_init(backend_t *backend){ // {{{
	hashtable_userdata    *userdata = backend->userdata = calloc(1, sizeof(hashtable_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->input = HK(key);
	return 0;
} // }}}
static int hashtable_destroy(backend_t *backend){ // {{{
	hashtable_userdata    *userdata = (hashtable_userdata *)backend->userdata;

	free(userdata);
	return 0;
} // }}}
static int hashtable_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	hashtable_userdata    *userdata        = (hashtable_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->input,          config, HK(input));
       	hash_data_copy(ret, TYPE_UINTT,    userdata->hashtable_size, config, HK(nelements));
	
	if(userdata->hashtable_size == 0)
		return error("invalid hashtable size");
	return 0;
} // }}}

static ssize_t hashtable_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uint32_t               action;
	uintmax_t              d_input;
	hashtable_userdata    *userdata          = (hashtable_userdata *)backend->userdata;

	hash_data_copy(ret, TYPE_UINT32T, action,  request, HK(action));         if(ret != 0) return -ENOSYS;
	hash_data_copy(ret, TYPE_UINTT,   d_input, request, userdata->input);
	if(ret == 0){
		d_input = d_input % userdata->hashtable_size;
		
		request_t r_next[] = {
			{ userdata->input,  DATA_PTR_UINTT(&d_input) },
			hash_next(request)
		};
		return ( (ret = backend_pass(backend, r_next)) < 0 ) ? ret : -EEXIST;
	}
	return ( (ret = backend_pass(backend, request)) < 0 ) ? ret : -EEXIST;
} // }}}

backend_t hashtable_proto = {
	.class          = "index/hashtable",
	.supported_api  = API_HASH,
	.func_init      = &hashtable_init,
	.func_configure = &hashtable_configure,
	.func_destroy   = &hashtable_destroy,
	.backend_type_hash = {
		.func_handler = &hashtable_handler
	}
};

