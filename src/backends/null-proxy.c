#include <libfrozen.h>
#include <backend.h>

/* this module is only for development */

static int null_init(chain_t *chain){
	return 0;
}

static int null_destroy(chain_t *chain){
	return 0;
}

static int null_configure(chain_t *chain, setting_t *config){
	return 0;
}


static int null_create(chain_t *chain, void *key, size_t value_size){
	if(value_size == 0x0000BEEF) // this check for backend tests
		return value_size;
	
	return chain_next_crwd_create(chain, key, value_size);
}

static int null_set(chain_t *chain, void *key, buffer_t *value, size_t value_size){
	return chain_next_crwd_set(chain, key, value, value_size);
}

static int null_get(chain_t *chain, void *key, buffer_t *value, size_t value_size){
	return chain_next_crwd_get(chain, key, value, value_size);
}

static int null_delete(chain_t *chain, void *key, size_t value_size){
	return chain_next_crwd_delete(chain, key, value_size);
}

static int null_move(chain_t *chain, void *key_from, void *key_to, size_t value_size){
	return chain_next_crwd_move(chain, key_from, key_to, value_size);
}

static int null_count(chain_t *chain, void *count){
	return chain_next_crwd_count(chain, count);
}

static chain_t chain_null_proxy = {
	"null-proxy",
	CHAIN_TYPE_CRWD,
	&null_init,
	&null_configure,
	&null_destroy,
	{
		&null_create,
		&null_set,
		&null_get,
		&null_delete,
		&null_move,
		&null_count
	}
};
CHAIN_REGISTER(chain_null_proxy)

