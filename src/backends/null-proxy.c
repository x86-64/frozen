#include <libfrozen.h>
#include <backend.h>

/* this module must be used only as example */

static int null_init(chain_t *chain){
	return 0;
}

static int null_destroy(chain_t *chain){
	return 0;
}

static int null_configure(chain_t *chain, setting_t *config){
	return 0;
}

static ssize_t null_create(chain_t *chain, request_t *request, buffer_t *buffer){
	unsigned int  value_size;
	
	if(hash_get_in_buf(request, "size", TYPE_INT32, &value_size) != 0)
		return -EINVAL;
		
	if(value_size == 0x0000BEEF) // this check for backend tests
		return value_size;
	
	return chain_next_query(chain, request, buffer);
}

static chain_t chain_null_proxy = {
	"null-proxy",
	CHAIN_TYPE_CRWD,
	&null_init,
	&null_configure,
	&null_destroy,
	{{
		.func_create = &null_create,
	}}
};
CHAIN_REGISTER(chain_null_proxy)

