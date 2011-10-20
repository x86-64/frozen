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
	hash_t                *r_next            = NULL;
	plode_userdata        *userdata          = (plode_userdata *)backend->userdata;
	
	if( (buffer = hash_data_find(request, userdata->buffer)) == NULL)
		return -EINVAL;
	
	data_convert(ret, TYPE_HASHT, r_next, buffer);
	if(ret != 0 || r_next == NULL)
		return -EFAULT;
	
	ret = ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
	
	hash_free(r_next);
	
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

