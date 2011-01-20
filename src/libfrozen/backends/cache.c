#include <libfrozen.h>
#include <backend.h>

typedef struct cache_userdata {
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
	DT_INT64         file_size;
	cache_userdata  *userdata   = (cache_userdata *)chain->userdata;
	
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
	
	memory_new(&userdata->memory, MEMORY_EXACT, ALLOC_MALLOC, 0, 1);
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

static ssize_t cache_backend_read(chain_t *chain, request_t *request){
	data_t         *buffer;
	data_ctx_t     *buffer_ctx;
	cache_userdata *userdata = (cache_userdata *)chain->userdata;
	
	buffer     = hash_get_data     (request, HK(buffer));
	buffer_ctx = hash_get_data_ctx (request, HK(buffer));
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	
	return data_transfer(buffer, buffer_ctx, &d_memory, request);
}

static ssize_t cache_backend_write(chain_t *chain, request_t *request){
	data_t         *buffer;
	data_ctx_t     *buffer_ctx;
	cache_userdata *userdata = (cache_userdata *)chain->userdata;
	
	buffer     = hash_get_data     (request, HK(buffer));
	buffer_ctx = hash_get_data_ctx (request, HK(buffer));
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	
	return data_transfer(&d_memory, request, buffer, buffer_ctx);
}

static ssize_t cache_backend_create(chain_t *chain, request_t *request){
	ssize_t         ret;
	DT_SIZET        size;
	DT_OFFT         offset;
	data_t          offset_data = DATA_PTR_OFFT(&offset);
	data_t         *offset_out;
	data_ctx_t     *offset_out_ctx;
	cache_userdata *userdata = (cache_userdata *)chain->userdata;
	
	hash_copy_data(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return -EINVAL;
	
	if(memory_grow(&userdata->memory, size, &offset) != 0)
		return -EFAULT;
	
	/* optional return of offset */
	offset_out     = hash_get_data    (request, HK(offset_out));
	offset_out_ctx = hash_get_data_ctx(request, HK(offset_out));
	data_transfer(offset_out, offset_out_ctx, &offset_data, NULL);
	
	/* optional write from buffer */
	request_t r_write[] = {
		{ HK(offset), offset_data },
		hash_next(request)
	};
	if( (ret = cache_backend_write(chain, r_write)) != -EINVAL)
		return ret;
	
	return 0;
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

