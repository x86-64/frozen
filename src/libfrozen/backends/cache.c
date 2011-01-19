#include <libfrozen.h>
#include <backend.h>

typedef struct cache_userdata {
	uint64_t    max_size;
	
	memory_t    memory;
} cache_userdata;

static int cache_init(chain_t *chain){ // {{{
	cache_userdata *userdata = chain->userdata = calloc(1, sizeof(cache_userdata));
	if(userdata == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
static int cache_destroy(chain_t *chain){ // {{{
	cache_userdata *userdata = (cache_userdata *)chain->userdata;
	
	data_t d_memory = DATA_MEMORYT(&userdata->memory);
	data_t d_chain  = DATA_CHAINT(chain);
	data_transfer(&d_chain, NULL, &d_memory, NULL);
	
	memory_free(&userdata->memory);
	free(userdata);
	return 0;
} // }}}
static int cache_configure(chain_t *chain, hash_t *config){ // {{{
	ssize_t          ret;
	DT_INT64         file_size;
	DT_INT64         max_size   = 0;
	cache_userdata  *userdata   = (cache_userdata *)chain->userdata;
	
	hash_copy_data(ret, TYPE_INT64, max_size, config, HK(max_size));
	userdata->max_size = max_size;
	
	/* get file size */
	request_t r_count[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_COUNT)   },
		{ HK(buffer), DATA_PTR_INT64(&file_size)      },
		hash_end
	};
	if(chain_next_query(chain, r_count) < 0)
		return -EFAULT;
	
	if(__MAX(size_t) < file_size)
		return 0;
	
	memory_new(&userdata->memory, MEMORY_EXACT, ALLOC_MALLOC, 0);
	if(memory_resize(&userdata->memory, (size_t)file_size) != 0)
		return 0;
	
	data_t d_memory = DATA_MEMORYT(&userdata->memory);
	data_t d_chain  = DATA_CHAINT(chain);
	if(data_transfer(&d_memory, NULL, &d_chain, NULL) < 0)
		goto error;
	
exit:
	return 0;
error:
	memory_free(&userdata->memory);
	goto exit;
} // }}}

static ssize_t cache_backend_create(chain_t *chain, request_t *request){
	ssize_t         ret;
	//cache_userdata *userdata = (cache_userdata *)chain->userdata;
	
	
	ret = chain_next_query(chain, request);
	
	return ret;
}

static ssize_t cache_backend_read(chain_t *chain, request_t *request){
/*	ssize_t         ret;
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
	
	if( (buffer = hash_get_data(request, HK(buffer))) == NULL)
		return -EINVAL;
	buffer_ctx = NULL; //hash_get_data_ctx(request, HK(buffer));
	
	ret = data_write(buffer, buffer_ctx, 0, userdata->cache + offset, size);
	return ret;
call_next:*/
	return chain_next_query(chain, request);	
}

static ssize_t cache_backend_write(chain_t *chain, request_t *request){
/*	ssize_t         ret;
	DT_INT64        offset;
	DT_SIZET        size;
	//data_t          offset_data = DATA_PTR_INT64(&offset);
	cache_userdata *userdata = (cache_userdata *)chain->userdata;
	data_t         *buffer;
	data_ctx_t     *buffer_ctx;
	
	if(userdata->cache == NULL)
		goto call_next;
	
	hash_copy_data(ret, TYPE_INT64, offset, request, HK(offset));
	hash_copy_data(ret, TYPE_SIZET, size,   request, HK(size)); // BUG not work if not know size
	
	if(offset >= userdata->cache_size)
		goto call_next;
	
	if( (buffer = hash_get_data(request, HK(buffer))) == NULL)
		return -EINVAL;
	buffer_ctx = NULL; //hash_get_data_ctx(request, HK(buffer));
	
	ret = data_read(buffer, buffer_ctx, 0, userdata->cache + offset, size);
	return ret;
call_next:*/
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

