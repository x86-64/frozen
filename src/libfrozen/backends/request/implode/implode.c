#include <libfrozen.h>

#define EMODULE 34

typedef struct plode_userdata {
	hash_key_t             buffer;
} plode_userdata;

static int plode_init(backend_t *backend){ // {{{
	plode_userdata        *userdata          = backend->userdata = calloc(1, sizeof(plode_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	userdata->buffer = HK(buffer);
	return 0;
} // }}}
static int plode_destroy(backend_t *backend){ // {{{
	plode_userdata          *userdata          = (plode_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int plode_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	plode_userdata        *userdata          = (plode_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->buffer, config, HK(buffer));
	return 0;
} // }}}

static ssize_t implode_request(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	plode_userdata        *userdata          = (plode_userdata *)backend->userdata;
	
	request_t r_next[] = {
		{ userdata->buffer, DATA_PTR_HASHT(request) },
		hash_end
	};
	return ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
} // }}}
static ssize_t explode_request(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *buffer;
	data_t                 r_hash            = DATA_PTR_HASHT(NULL);
	plode_userdata        *userdata          = (plode_userdata *)backend->userdata;
	
	if( (buffer = hash_data_find(request, userdata->buffer)) == NULL)
		return -EINVAL;
	
	fastcall_convert  r_convert = { { 4, ACTION_CONVERT }, &r_hash, FORMAT_BINARY };
	if( (ret = data_query(buffer, &r_convert)) < 0)
		return -EFAULT;
	
	if(r_hash.ptr == NULL)
		return -EFAULT;
	
	ret = ( (ret = backend_pass(backend, r_hash.ptr)) < 0) ? ret : -EEXIST;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&r_hash, &r_free);
	
	return ret;
} // }}}

backend_t implode_proto = {
	.class          = "request/implode",
	.supported_api  = API_HASH,
	.func_init      = &plode_init,
	.func_configure = &plode_configure,
	.func_destroy   = &plode_destroy,
	.backend_type_hash = {
		.func_handler = &implode_request,
	}
};

backend_t explode_proto = {
	.class          = "request/explode",
	.supported_api  = API_HASH,
	.func_init      = &plode_init,
	.func_configure = &plode_configure,
	.func_destroy   = &plode_destroy,
	.backend_type_hash = {
		.func_handler = &explode_request,
	}
};

