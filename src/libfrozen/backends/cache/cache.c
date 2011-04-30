#include <libfrozen.h>
#include <backend.h>
#include <pthread.h>

/**
 * @file cache.c
 * @ingroup modules
 * @brief Cache module
 *
 * Cache module hold data in memory, instead of underlying backend
 */
/**
 * @ingroup modules
 * @addtogroup mod_cache Module 'cache'
 */
/**
 * @ingroup mod_cache
 * @page page_cache_config Cache configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class         = "cache"
 * 	}
 *
 * @endcode
 */

#define EMODULE 3
#define CACHE_PAGE_SIZE      0x1000

typedef struct cache_userdata {
	uintmax_t        enabled;
	memory_t         memory;
	
	// locks
	pthread_mutex_t  resize_lock;
	pthread_rwlock_t write_lock;
	
} cache_userdata;

static uintmax_t cache_get_filesize(backend_t *backend){ // {{{
	ssize_t                ret;
	uintmax_t              file_size = 0;
	
	/* get file size */
	request_t r_count[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_COUNT)   },
		{ HK(buffer), DATA_PTR_UINTT(&file_size)        },
		{ HK(ret),    DATA_PTR_SIZET(&ret)              },
		hash_end
	};
	if(backend_pass(backend, r_count) < 0 || ret < 0)
		return 0;
	
	return file_size;
} // }}}
static void      cache_enable(backend_t *backend){ // {{{
	uintmax_t              file_size;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_mutex_lock(&userdata->resize_lock);
		
		if(userdata->enabled == 1)
			goto exit;
		
		file_size = cache_get_filesize(backend);
		
		// alloc new memory
		memory_new(&userdata->memory, MEMORY_PAGE, ALLOC_MALLOC, CACHE_PAGE_SIZE, 1);
		if(memory_resize(&userdata->memory, file_size) != 0)
			goto failed;
		
		pthread_rwlock_wrlock(&userdata->write_lock); // prevent data update in backend
			
			// read data from backend to new memory
			data_t d_memory   = DATA_MEMORYT(&userdata->memory);
			data_t d_backend  = DATA_BACKENDT(backend);
			if(data_transfer(&d_memory, NULL, &d_backend, NULL) < 0)
				goto failed;
			
			// enable caching
			userdata->enabled = 1;
			
		pthread_rwlock_unlock(&userdata->write_lock);
		
exit:
	pthread_mutex_unlock(&userdata->resize_lock);
	return;
failed:
	memory_free(&userdata->memory);
	goto exit;
} // }}}
static void      cache_disable(backend_t *backend){ // {{{
	uintmax_t              file_size;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_mutex_lock(&userdata->resize_lock);
	
		if(userdata->enabled == 0)
			goto exit;
		
		if(memory_size(&userdata->memory, &file_size) < 0)
			goto exit;
		
		pthread_rwlock_wrlock(&userdata->write_lock); // prevent writing new data
			
			// flush data from memory to backend
			data_t d_memory  = DATA_MEMORYT(&userdata->memory);
			data_t d_backend = DATA_BACKENDT(backend);
			data_transfer(&d_backend, NULL, &d_memory, NULL);
			
			// free used memory
			memory_free(&userdata->memory);
			
			// disable caching
			userdata->enabled = 0;
			
		pthread_rwlock_unlock(&userdata->write_lock);
		
exit:
	pthread_mutex_unlock(&userdata->resize_lock);
} // }}}

static int cache_init(backend_t *backend){ // {{{
	cache_userdata        *userdata          = backend->userdata = calloc(1, sizeof(cache_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int cache_destroy(backend_t *backend){ // {{{
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	cache_disable(backend);
	pthread_mutex_destroy(&userdata->resize_lock);
	pthread_rwlock_destroy(&userdata->write_lock);
	
	free(userdata);
	return 0;
} // }}}
static int cache_configure(backend_t *backend, config_t *config){ // {{{
	pthread_mutexattr_t    attr; 
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_mutexattr_init(&attr); 
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&userdata->resize_lock, &attr);
	pthread_mutexattr_destroy(&attr);
	
	pthread_rwlock_init(&userdata->write_lock, NULL);
	
	// try enable cache
	cache_enable(backend);
	
	return 0;
} // }}}

static ssize_t cache_backend_read(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *buffer;
	data_ctx_t            *buffer_ctx;
	cache_userdata        *userdata = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 0)
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	// perform request
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	
	return data_transfer(buffer, buffer_ctx, &d_memory, request);
} // }}}
static ssize_t cache_backend_write(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *buffer;
	data_ctx_t            *buffer_ctx;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_rwlock_rdlock(&userdata->write_lock); // read lock, many requests allowed
		
		if(userdata->enabled == 0){
			ret = ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
		}else{
			hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
			
			data_t d_memory = DATA_MEMORYT(&userdata->memory);
			
			ret = data_transfer(&d_memory, request, buffer, buffer_ctx);
		}
	
	pthread_rwlock_unlock(&userdata->write_lock);
	
	return ret;
} // }}}
static ssize_t cache_backend_create(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	size_t                 size;
	off_t                  offset;
	data_t                 offset_data       = DATA_PTR_OFFT(&offset);
	data_t                *offset_out;
	data_ctx_t            *offset_out_ctx;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 0)
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	hash_data_copy(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return warning("no size supplied");
	
	pthread_mutex_lock(&userdata->resize_lock);
		
		if(memory_grow(&userdata->memory, size, &offset) != 0){
			ret = error("memory_grow failed");
		}
		
	pthread_mutex_unlock(&userdata->resize_lock);
	
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
} // }}}
static ssize_t cache_backend_delete(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	off_t                  offset;
	size_t                 size;
	uintmax_t              mem_size;
	cache_userdata         *userdata         = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 0){
		// TODO add random check if file less than limits now
		
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	}
	
	// TIP cache, as like as file, can only truncate
	
	hash_data_copy(ret, TYPE_OFFT,  offset, request, HK(offset));  if(ret != 0) return warning("offset not supplied");
	hash_data_copy(ret, TYPE_SIZET, size,   request, HK(size));    if(ret != 0) return warning("size not supplied");
	
	pthread_mutex_lock(&userdata->resize_lock);
		
		if(memory_size(&userdata->memory, &mem_size) < 0){
			ret = error("memory_size failed");
			goto exit;
		}
		
		if(offset + size != mem_size){ // truncating only last elements
			ret = warning("cant delete not last elements");
			goto exit;
		}
		
		if(memory_resize(&userdata->memory, (uintmax_t)offset) != 0){
			ret = error("memory_resize failed");
			goto exit;
		}
		
exit:		
	pthread_mutex_unlock(&userdata->resize_lock);
	
	return 0; //-EFAULT;
} // }}}
static ssize_t cache_backend_count(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *buffer;
	data_ctx_t            *buffer_ctx;
	uintmax_t              size;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 0)
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	if(memory_size(&userdata->memory, &size) < 0)
		return error("memory_size failed");
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	data_t count = DATA_PTR_UINTT(&size);
	
	return data_transfer(
		buffer, buffer_ctx,
		&count,  NULL
	);
} // }}}

/*
typedef ssize_t (*f_fast_create) (backend_t *, off_t *, size_t);
typedef ssize_t (*f_fast_read)   (backend_t *, off_t, void *, size_t);
typedef ssize_t (*f_fast_write)  (backend_t *, off_t, void *, size_t);
typedef ssize_t (*f_fast_delete) (backend_t *, off_t, size_t);
*/

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

