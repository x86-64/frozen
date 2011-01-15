#include <libfrozen.h>
#include <backend.h>

typedef struct cache_userdata {
	uint64_t    max_size;
	uint64_t    cache_size;
	void       *cache;
} cache_userdata;

/* cache functions {{{ */
static void    cache_free(cache_userdata *userdata){
	free(userdata->cache);
	userdata->cache      = NULL;
	userdata->cache_size = 0;
}

static ssize_t cache_flush(chain_t *chain, cache_userdata *userdata){
	if(userdata->cache == NULL)
		return 0;
	
	request_t r_write[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_WRITE)                   },
		{ HK(offset), DATA_OFFT(0)                                    },
		{ HK(buffer), DATA_MEM(userdata->cache, userdata->cache_size) },
	//	{ HK(size),   DATA_PTR_INT64(&userdata->cache_size)           },
		hash_end
	};
	return chain_next_query(chain, r_write);
}

static ssize_t cache_read_all(chain_t *chain, cache_userdata *userdata){
	userdata->cache      = NULL;
	userdata->cache_size = 0;
	
	request_t r_count[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_COUNT)         },
		{ HK(buffer), DATA_PTR_INT64(&userdata->cache_size) },
		hash_end
	};
	if(chain_next_query(chain, r_count) < 0)
		return -EFAULT;
	
	if(userdata->cache_size >= userdata->max_size && userdata->max_size != 0)
		return -EFAULT;
	
	if( (userdata->cache = malloc(userdata->cache_size)) == NULL)
		return -EFAULT;
	
	memset(userdata->cache, 0, userdata->cache_size);
	
	request_t r_read[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_READ)                    },
		{ HK(offset), DATA_OFFT(0)                                    },
		{ HK(buffer), DATA_MEM(userdata->cache, userdata->cache_size) },
		hash_end
	};
	if(chain_next_query(chain, r_read) < 0)
		goto error;
	
	return 0;
error:
	cache_free(userdata);
	return -EFAULT;
}
/* }}} */

static int cache_init(chain_t *chain){ // {{{
	cache_userdata *userdata = chain->userdata = calloc(1, sizeof(cache_userdata));
	if(userdata == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
static int cache_destroy(chain_t *chain){ // {{{
	cache_userdata *userdata = (cache_userdata *)chain->userdata;
	
	cache_flush(chain, userdata);
	cache_free(userdata);
	
	free(userdata);
	return 0;
} // }}}
static int cache_configure(chain_t *chain, hash_t *config){ // {{{
	ssize_t          ret;
	cache_userdata  *userdata   = (cache_userdata *)chain->userdata;
	DT_INT64         max_size   = 0;
	
	hash_copy_data(ret, TYPE_INT64, max_size, config, HK(max_size));
	
	userdata->max_size = max_size;
	
	cache_read_all(chain, userdata);
	return 0;
} // }}}

static ssize_t cache_backend_create(chain_t *chain, request_t *request){
	ssize_t         ret;
	cache_userdata *userdata = (cache_userdata *)chain->userdata;
	
	if(userdata->cache != NULL){
		cache_flush    (chain, userdata);
		cache_free     (userdata);
	}
	
	ret = chain_next_query(chain, request);
	
	cache_read_all (chain, userdata);
	
	return ret;
}

static ssize_t cache_backend_read(chain_t *chain, request_t *request){
	ssize_t         ret;
	DT_INT64        offset;
	DT_SIZET        size;
	//data_t          offset_data = DATA_PTR_INT64(&offset);
	cache_userdata *userdata = (cache_userdata *)chain->userdata;
	data_t         *buffer;
	data_ctx_t     *buffer_ctx;
	
	if(userdata->cache == NULL)
		goto call_next;
	
	hash_copy_data(ret, TYPE_INT64, offset, request, HK(offset));
	hash_copy_data(ret, TYPE_SIZET, size,   request, HK(size));
	
	if(offset >= userdata->cache_size)
		goto call_next;
	
	/* get buffer info */
	if( (buffer = hash_get_data(request, HK(buffer))) == NULL)
		return -EINVAL;
	buffer_ctx = NULL; //hash_get_data_ctx(request, HK(buffer));
	
	ret = data_write(buffer, buffer_ctx, 0, userdata->cache + offset, size);
	return ret;
call_next:
	return chain_next_query(chain, request);	
}

static ssize_t cache_backend_write(chain_t *chain, request_t *request){
	ssize_t         ret;
	DT_INT64        offset;
	DT_SIZET        size;
	//data_t          offset_data = DATA_PTR_INT64(&offset);
	cache_userdata *userdata = (cache_userdata *)chain->userdata;
	data_t         *buffer;
	data_ctx_t     *buffer_ctx;
	
	if(userdata->cache == NULL)
		goto call_next;
	
	hash_copy_data(ret, TYPE_INT64, offset, request, HK(offset));
	hash_copy_data(ret, TYPE_SIZET, size,   request, HK(size));
	
	if(offset >= userdata->cache_size)
		goto call_next;
	
	/* get buffer info */
	if( (buffer = hash_get_data(request, HK(buffer))) == NULL)
		return -EINVAL;
	buffer_ctx = NULL; //hash_get_data_ctx(request, HK(buffer));
	
	ret = data_read(buffer, buffer_ctx, 0, userdata->cache + offset, size);
	return ret;
call_next:
	return chain_next_query(chain, request);	
}

// TODO delete

static chain_t chain_cache = {
	"cache",
	CHAIN_TYPE_CRWD,
	.func_init      = &cache_init,
	.func_configure = &cache_configure,
	.func_destroy   = &cache_destroy,
	{{
		.func_create = &cache_backend_create,
		.func_get    = &cache_backend_read,
		.func_set    = &cache_backend_write
	}}
};
CHAIN_REGISTER(chain_cache)

