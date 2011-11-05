#include <libfrozen.h>
#include <backend.h>
#include <pthread.h>

/**
 * @ingroup backend
 * @addtogroup mod_cache Backend 'cache/cache'
 *
 * Cache module hold data in memory, instead of underlying backend
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
	uintmax_t              enabled;
	memory_t               memory;
	data_t                 d_memory;
	pthread_rwlock_t       lock;
} cache_userdata;

static void cache_enable  (backend_t *backend);
static void cache_disable (backend_t *backend);

static size_t  cache_fast_create (backend_t *backend, off_t *offset, size_t size){ // {{{
	size_t                 ret               = size;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_rwlock_wrlock(&userdata->lock);
		
		if(memory_grow(&userdata->memory, size, offset) != 0)
			ret = 0;
		
	pthread_rwlock_unlock(&userdata->lock);
	return ret;
} // }}}
static size_t  cache_fast_delete (backend_t *backend, off_t offset, size_t size){ // {{{
	uintmax_t              mem_size;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_rwlock_wrlock(&userdata->lock);
		
		if(memory_size(&userdata->memory, &mem_size) < 0)
			goto error;
		
		if(offset + size != mem_size) // truncating only last elements
			goto error;
		
		if(memory_resize(&userdata->memory, (uintmax_t)offset) != 0)
			goto error;
	
	pthread_rwlock_unlock(&userdata->lock);
	return size;
	
error:		
	pthread_rwlock_unlock(&userdata->lock);
	return 0;
} // }}}
/*
static size_t  cache_fast_read   (backend_t *backend, off_t offset, void *buffer, size_t size){ // {{{
	size_t                 ret;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_rwlock_rdlock(&userdata->lock);
		
		ret = memory_read( &userdata->memory, offset, buffer, size);
	
	pthread_rwlock_unlock(&userdata->lock);
	return ret;
} // }}}
static size_t  cache_fast_write  (backend_t *backend, off_t offset, void *buffer, size_t size){ // {{{
	size_t                 ret;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_rwlock_rdlock(&userdata->lock);
		
		ret = memory_write( &userdata->memory, offset, buffer, size);
	
	pthread_rwlock_unlock(&userdata->lock);
	return ret;
} // }}}
*/

static ssize_t cache_backend_read(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              offset            = 0;
	uintmax_t              size              = ~0;
	data_t                *buffer;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_OFFT,  offset, request, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, size,   request, HK(size));
	
	buffer = hash_data_find(request, HK(buffer));
	
	pthread_rwlock_rdlock(&userdata->lock);
	
		data_t d_slice = DATA_SLICET(&userdata->d_memory, offset, size);
		
		fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, buffer };
		ret = data_query(&d_slice, &r_transfer);
	
	pthread_rwlock_unlock(&userdata->lock);
	
	return ret;
} // }}}
static ssize_t cache_backend_write(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              offset            = 0;
	uintmax_t              size              = ~0;
	data_t                *buffer;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_OFFT,  offset, request, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, size,   request, HK(size));
	
	buffer = hash_data_find(request, HK(buffer));
	
	pthread_rwlock_rdlock(&userdata->lock); // read lock, many requests allowed
		
		data_t d_slice = DATA_SLICET(&userdata->d_memory, offset, size);
		
		fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_slice };
		ret = data_query(buffer, &r_transfer);
	
	pthread_rwlock_unlock(&userdata->lock);
	
	return ret;
} // }}}
static ssize_t cache_backend_create(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	size_t                 size;
	off_t                  offset;
	data_t                 offset_data       = DATA_PTR_OFFT(&offset);
	data_t                *offset_out;
	
	hash_data_copy(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return warning("no size supplied");
	
	if( (cache_fast_create(backend, &offset, size) != size) )
		return error("memory_grow failed");
	
	/* optional return of offset */
	offset_out = hash_data_find(request, HK(offset_out));
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, offset_out };
	data_query(&offset_data, &r_transfer);
	
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
	
	// TIP cache, as like as file, can only truncate
	
	hash_data_copy(ret, TYPE_OFFT,  offset, request, HK(offset));  if(ret != 0) return warning("offset not supplied");
	hash_data_copy(ret, TYPE_SIZET, size,   request, HK(size));    if(ret != 0) return warning("size not supplied");
	
	if( cache_fast_delete(backend, offset, size) != size )
		return error("cache delete failed");
	
	return 0;
} // }}}
static ssize_t cache_backend_count(backend_t *backend, request_t *request){ // {{{
	data_t                *buffer;
	uintmax_t              size;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_rwlock_rdlock(&userdata->lock);
	
		if(memory_size(&userdata->memory, &size) < 0)
			return error("memory_size failed");
	
	pthread_rwlock_unlock(&userdata->lock);
	
	data_t count = DATA_PTR_UINTT(&size);
	
	buffer = hash_data_find(request, HK(buffer));
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, buffer };
	return data_query(&count, &r_transfer);
} // }}}

static uintmax_t cache_get_filesize(backend_t *backend){ // {{{
	ssize_t                ret;
	uintmax_t              file_size = 0;
	
	/* get file size */
	request_t r_count[] = {
		{ HK(action), DATA_UINT32T(ACTION_COUNT)   },
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
	backend_t             *child;
	void                  *iter              = NULL;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_rwlock_wrlock(&userdata->lock);
		
		if(userdata->enabled == 1)
			goto exit;
		
		file_size = cache_get_filesize(backend);
		
		// alloc new memory
		memory_new(&userdata->memory, MEMORY_PAGE, ALLOC_MALLOC, CACHE_PAGE_SIZE, 1);
		if(memory_resize(&userdata->memory, file_size) != 0)
			goto failed;
		
		// read data from backend to new memory
		data_t d_memory   = DATA_MEMORYT(&userdata->memory);
		while( (child = list_iter_next(&backend->childs, &iter)) != NULL){
			data_t d_backend  = DATA_BACKENDT(backend);
			
			fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_memory };
			if(data_query(&d_backend, &r_transfer) < 0)
				goto failed;
		}
		userdata->d_memory = d_memory;
		
		// enable caching
		userdata->enabled = 1;
		backend->backend_type_crwd.func_create      = &cache_backend_create;
		backend->backend_type_crwd.func_get         = &cache_backend_read;
		backend->backend_type_crwd.func_set         = &cache_backend_write;
		backend->backend_type_crwd.func_delete      = &cache_backend_delete;
		backend->backend_type_crwd.func_count       = &cache_backend_count;
		
exit:
	pthread_rwlock_unlock(&userdata->lock);
	return;
failed:
	memory_free(&userdata->memory);
	goto exit;
} // }}}
static void      cache_disable(backend_t *backend){ // {{{
	uintmax_t              file_size;
	backend_t             *child;
	void                  *iter              = NULL;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_rwlock_wrlock(&userdata->lock);
	
		if(userdata->enabled == 0)
			goto exit;
		
		if(memory_size(&userdata->memory, &file_size) < 0)
			goto exit;
		
		// flush data from memory to backend
		data_t d_memory  = DATA_MEMORYT(&userdata->memory);
		while( (child = list_iter_next(&backend->childs, &iter)) != NULL){
			data_t d_backend = DATA_BACKENDT(child);
			
			fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_backend };
			data_query(&d_memory, &r_transfer);
		}

		// free used memory
		memory_free(&userdata->memory);
		
		// disable caching
		userdata->enabled = 0;
		backend->backend_type_crwd.func_create      = NULL;
		backend->backend_type_crwd.func_get         = NULL;
		backend->backend_type_crwd.func_set         = NULL;
		backend->backend_type_crwd.func_delete      = NULL;
		backend->backend_type_crwd.func_count       = NULL;
		
exit:
	pthread_rwlock_unlock(&userdata->lock);
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
	pthread_rwlock_destroy(&userdata->lock);
	
	free(userdata);
	return 0;
} // }}}
static int cache_configure(backend_t *backend, config_t *config){ // {{{
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_rwlock_init(&userdata->lock, NULL);
	cache_enable(backend);                               // try enable cache
	
	return 0;
} // }}}

backend_t cache_proto = {
	.class          = "cache/cache",
	.supported_api  = API_CRWD,
	.func_init      = &cache_init,
	.func_configure = &cache_configure,
	.func_destroy   = &cache_destroy,
	{
		.func_create      = NULL,
		.func_get         = NULL,
		.func_set         = NULL,
		.func_delete      = NULL,
		.func_count       = NULL
	}
};

