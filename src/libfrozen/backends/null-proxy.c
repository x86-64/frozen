#include <libfrozen.h>

/* this module must be used only as example */

static int null_init(chain_t *chain){
	(void)chain;
	return 0;
}

static int null_destroy(chain_t *chain){
	(void)chain;
	return 0;
}

static int null_configure(chain_t *chain, hash_t *config){
	(void)chain; (void)config;
	return 0;
}

static ssize_t null_create(chain_t *chain, request_t *request){
	ssize_t       ret;
	size_t        value;
	
	hash_data_copy(ret, TYPE_SIZET, value, request, HK(size));
	if(ret == 0 && value == 0x0000BEEF)
		return value;
	
	return chain_next_query(chain, request);
}

static chain_t chain_null_proxy = {
	"null-proxy",
	CHAIN_TYPE_CRWD,
	.func_init      = &null_init,
	.func_configure = &null_configure,
	.func_destroy   = &null_destroy,
	{{
		.func_create = &null_create,
	}}
};
CHAIN_REGISTER(chain_null_proxy)

