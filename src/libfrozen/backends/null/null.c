#include <libfrozen.h>

/* this module must be used only as example */

static int null_init(backend_t *backend){
	(void)backend;
	return 0;
}

static int null_destroy(backend_t *backend){
	(void)backend;
	return 0;
}

static int null_configure(backend_t *backend, hash_t *config){
	(void)backend; (void)config;
	return 0;
}

static ssize_t null_create(backend_t *backend, request_t *request){
	ssize_t       ret;
	size_t        value;
	
	hash_data_copy(ret, TYPE_SIZET, value, request, HK(size));
	if(ret == 0 && value == 0x0000BEEF)
		return value;
	
	return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
}

backend_t null_proto = {
	.class          = "null",
	.supported_api  = API_CRWD,
	.func_init      = &null_init,
	.func_configure = &null_configure,
	.func_destroy   = &null_destroy,
	{
		.func_create = &null_create,
	}
};


