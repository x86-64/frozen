#include <libfrozen.h>
#include <shop/pass/pass.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_pool machine/pool
 *
 * This module keep pool of given shops and manage them.
 */
/**
 * @ingroup mod_machine_pool
 * @page page_pool_config Configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class             = "machine/pool",
 * 	        config            = { ... },               // hash with shop configuration
 * 	        size              = (uint_t)"10"           // initial pool size, default 1
 * 	}
 * @endcode
 */

#define ERRORS_MODULE_ID 14
#define ERRORS_MODULE_NAME "machine/pool"

typedef struct pool_userdata {
	list                   shops;
	request_t             *prototype;
} pool_userdata;

static ssize_t pool_resize(machine_t *pool, uintmax_t new_size){ // {{{
	ssize_t                ret;
	list                  *shop;
	uintmax_t              current_size;
	request_t             *prototype;
	pool_userdata         *userdata          = (pool_userdata *)pool->userdata;
	
	current_size = list_count(&userdata->shops);
	
	for(; current_size < new_size; current_size++){
		prototype = hash_copy(userdata->prototype);
		
		ret = shop_new(prototype, &shop);
		
		hash_free(prototype);
		if(ret < 0)
			return ret;
		
		list_add(&userdata->shops, shop);
	}
	for(; current_size > new_size; current_size--){
		shop = list_pop(&userdata->shops);
		
		shop_list_destroy(shop);
	}
	return 0;
} // }}}

static ssize_t pool_init(machine_t *machine){ // {{{
	pool_userdata         *userdata          = machine->userdata = calloc(1, sizeof(pool_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	list_init(&userdata->shops);
	return 0;
} // }}}
static ssize_t pool_destroy(machine_t *machine){ // {{{
	pool_userdata         *userdata          = (pool_userdata *)machine->userdata;
	
	pool_resize(machine, 0);
	list_destroy(&userdata->shops);
	return 0;
} // }}}
static ssize_t pool_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              initial_size      = 1;
	pool_userdata         *userdata          = (pool_userdata *)machine->userdata;
	
	hash_data_get    (ret, TYPE_UINTT, initial_size,        config, HK(size));
	hash_data_consume(ret, TYPE_HASHT, userdata->prototype, config, HK(config));
	if(ret != 0)
		return error("config not supplied");
	
	pool_resize(machine, initial_size);
	return 0;
} // }}}

static ssize_t pool_handler(machine_t *machine, request_t *request){ // {{{
	return errorn(EBADF);
} // }}}

machine_t pool_proto = {
	.class          = "machine/pool",
	.supported_api  = API_HASH,
	.func_init      = &pool_init,
	.func_configure = &pool_configure,
	.func_destroy   = &pool_destroy,
	.machine_type_hash = {
		.func_handler = &pool_handler
	}
};

