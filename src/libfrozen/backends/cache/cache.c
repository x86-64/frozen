#include <libfrozen.h>
#include <backend.h>
#define EMODULE 3

typedef struct cache_userdata {
	memory_t         memory;
	pthread_mutex_t  create_lock;
} cache_userdata;

static int cache_init(backend_t *backend){ // {{{
	cache_userdata *userdata = backend->userdata = calloc(1, sizeof(cache_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int cache_destroy(backend_t *backend){ // {{{
	cache_userdata *userdata = (cache_userdata *)backend->userdata;
	
	data_t d_memory = DATA_MEMORYT(&userdata->memory);
	data_t d_backend  = DATA_BACKENDT(backend);
	data_transfer(&d_backend, NULL, &d_memory, NULL);
	
	memory_free(&userdata->memory);
	pthread_mutex_destroy(&userdata->create_lock);
	
	free(userdata);
	return 0;
} // }}}
static int cache_configure(backend_t *backend, hash_t *config){ // {{{
	size_t                 ret;
	uint64_t               file_size;
	cache_userdata        *userdata   = (cache_userdata *)backend->userdata;
	
	/* get file size */
	request_t r_count[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_COUNT)   },
		{ HK(buffer), DATA_PTR_UINT64T(&file_size)      },
		{ HK(ret),    DATA_PTR_SIZET(&ret)              },
		hash_end
	};
	if(backend_pass(backend, r_count) < 0 || ret < 0)
		return error("count failed");
	
	if(__MAX(size_t) < file_size)
		return 0;
	
	memory_new(&userdata->memory, MEMORY_PAGE, ALLOC_MALLOC, 0x1000, 1);
	if(memory_resize(&userdata->memory, (size_t)file_size) != 0)
		return 0;
	
	data_t d_memory   = DATA_MEMORYT(&userdata->memory);
	data_t d_backend  = DATA_BACKENDT(backend);
	if(data_transfer(&d_memory, NULL, &d_backend, NULL) < 0)
		goto error;
	
	pthread_mutex_init(&userdata->create_lock, NULL);
exit:
	return 0;
error: // TODO wrong destroy
	memory_free(&userdata->memory);
	goto exit;
} // }}}

static ssize_t cache_backend_read(backend_t *backend, request_t *request){
	data_t         *buffer;
	data_ctx_t     *buffer_ctx;
	cache_userdata *userdata = (cache_userdata *)backend->userdata;
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	
	return data_transfer(buffer, buffer_ctx, &d_memory, request);
}

static ssize_t cache_backend_write(backend_t *backend, request_t *request){
	data_t         *buffer;
	data_ctx_t     *buffer_ctx;
	cache_userdata *userdata = (cache_userdata *)backend->userdata;
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	
	return data_transfer(&d_memory, request, buffer, buffer_ctx);
}

static ssize_t cache_backend_create(backend_t *backend, request_t *request){
	ssize_t         ret;
	DT_SIZET        size;
	DT_OFFT         offset;
	data_t          offset_data = DATA_PTR_OFFT(&offset);
	data_t         *offset_out;
	data_ctx_t     *offset_out_ctx;
	cache_userdata *userdata = (cache_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return warning("no size supplied");
	
	pthread_mutex_lock(&userdata->create_lock);
	
		if(memory_grow(&userdata->memory, size, &offset) != 0)
			ret = error("memory_grow failed");
	
	pthread_mutex_unlock(&userdata->create_lock);
	
	if(ret != 0)
		return ret;
	
	/* optional return of offset */
	hash_data_find(request, HK(offset_out), &offset_out, &offset_out_ctx);
	data_transfer(offset_out, offset_out_ctx, &offset_data, NULL);
	
	/* optional write from buffer */
	request_t r_write[] = {
		{ HK(offset), offset_data },
		hash_next(request)
	};
	if( (ret = cache_backend_write(backend, r_write)) != -EINVAL)
		return ret;
	
	return 0;
}

static ssize_t cache_backend_delete(backend_t *backend, request_t *request){
	ssize_t                ret;
	off_t                  offset;
	size_t                 size, mem_size;
	cache_userdata         *userdata = (cache_userdata *)backend->userdata;
	
	// TIP cache, as like as file, can only truncate
	
	hash_data_copy(ret, TYPE_OFFT,  offset, request, HK(offset));  if(ret != 0) return warning("offset not supplied");
	hash_data_copy(ret, TYPE_SIZET, size,   request, HK(size));    if(ret != 0) return warning("size not supplied");
	
	pthread_mutex_lock(&userdata->create_lock);
		
		if(memory_size(&userdata->memory, &mem_size) < 0){
			ret = error("memory_size failed");
			goto exit;
		}
		
		if(offset + size != mem_size){ // truncating only last elements
			ret = warning("cant delete not last elements");
			goto exit;
		}
		
		if(memory_resize(&userdata->memory, (size_t)offset) != 0){
			ret = error("memory_resize failed");
			goto exit;
		}
exit:		
	pthread_mutex_unlock(&userdata->create_lock);
	
	return 0; //-EFAULT;
}

static ssize_t cache_backend_count(backend_t *backend, request_t *request){
	data_t         *buffer;
	data_ctx_t     *buffer_ctx;
	size_t          size;
	cache_userdata *userdata = (cache_userdata *)backend->userdata;
	
	if(memory_size(&userdata->memory, &size) < 0)
		return error("memory_size failed");
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	data_t count = DATA_PTR_SIZET(&size);
	
	return data_transfer(
		buffer, buffer_ctx,
		&count,  NULL
	);
}

backend_t cache_proto = {
	.class          = "cache",
	.supported_api  = API_CRWD,
	.func_init      = &cache_init,
	.func_configure = &cache_configure,
	.func_destroy   = &cache_destroy,
	{{
		.func_create = &cache_backend_create,
		.func_get    = &cache_backend_read,
		.func_set    = &cache_backend_write,
		.func_delete = &cache_backend_delete,
		.func_count  = &cache_backend_count
	}}
};


