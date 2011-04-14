#include <libfrozen.h>
#include <backend.h>
#include <pthread.h>

#define EMODULE 3
#define CACHE_PAGE_SIZE 0x1000

typedef enum usage_dir {
	DIR_INC,
	DIR_DEC,
	DIR_NONE
} usage_dir;

typedef struct cache_userdata {
	uintmax_t        enabled;
	memory_t         memory;
	pthread_mutex_t  lock;
	uintmax_t        memusage;
	
	pthread_mutex_t *perfork_lock;
	uintmax_t       *perfork_memusage;
	
	uintmax_t        limit_perfile;
	uintmax_t        limit_perfork;
	uintmax_t        limit_global;
} cache_userdata;

uintmax_t        memusage_global;
pthread_mutex_t  memusage_global_mtx = PTHREAD_MUTEX_INITIALIZER; 

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
static void      cache_memusage(backend_t *backend, usage_dir dir, uintmax_t value, uintmax_t *mem_global, uintmax_t *mem_fork){ // {{{
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_mutex_lock(&memusage_global_mtx);
		
		switch(dir){
			case DIR_INC:  memusage_global += value; break;
			case DIR_DEC:  memusage_global -= value; break;
			case DIR_NONE: break;
		};
		if(mem_global)
			*mem_global = memusage_global;
	
	pthread_mutex_unlock(&memusage_global_mtx);
	
	pthread_mutex_lock(userdata->perfork_lock);
		
		switch(dir){
			case DIR_INC:  *(userdata->perfork_memusage) += value; break;
			case DIR_DEC:  *(userdata->perfork_memusage) -= value; break;
			case DIR_NONE: break;
		}
		if(mem_fork)
			*mem_fork = *(userdata->perfork_memusage);
	
	pthread_mutex_unlock(userdata->perfork_lock);
} // }}}
static void      cache_enable(backend_t *backend){ // {{{
	uintmax_t              file_size;
	cache_userdata        *userdata   = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 1)
		return;
	
	pthread_mutex_lock(&userdata->lock);
		
		file_size = cache_get_filesize(backend);
		
		memory_new(&userdata->memory, MEMORY_PAGE, ALLOC_MALLOC, CACHE_PAGE_SIZE, 1);
		if(memory_resize(&userdata->memory, file_size) != 0)
			goto failed;
		
		data_t d_memory   = DATA_MEMORYT(&userdata->memory);
		data_t d_backend  = DATA_BACKENDT(backend);
		if(data_transfer(&d_memory, NULL, &d_backend, NULL) < 0)
			goto failed;
		
		userdata->enabled = 1;
		
		cache_memusage(backend, DIR_INC, file_size, NULL, NULL);
exit:
	pthread_mutex_unlock(&userdata->lock);
	return;
failed:
	memory_free(&userdata->memory);
	goto exit;
} // }}}
static void      cache_disable(backend_t *backend){ // {{{
	uintmax_t              file_size;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 0)
		return;
	
	pthread_mutex_lock(&userdata->lock);
	
		if(memory_size(&userdata->memory, &file_size) < 0)
			goto exit;
		
		// flush data
		data_t d_memory  = DATA_MEMORYT(&userdata->memory);
		data_t d_backend = DATA_BACKENDT(backend);
		data_transfer(&d_backend, NULL, &d_memory, NULL);
		
		// free mem
		memory_free(&userdata->memory);
		
		userdata->enabled = 0;
		
		cache_memusage(backend, DIR_DEC, file_size, NULL, NULL);
exit:
	pthread_mutex_unlock(&userdata->lock);
} // }}}
static void      cache_check(backend_t *backend, uintmax_t mem_file){ // {{{
	uintmax_t              mem_global;
	uintmax_t              mem_fork;
	uintmax_t              verdict           = 1; // enable by default
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	cache_memusage(backend, DIR_NONE, 0, &mem_global, &mem_fork);
	
	if(mem_file > userdata->limit_perfile)
		verdict = 0;
	
	if(mem_fork > userdata->limit_perfork)
		verdict = 0;
	
	if(mem_global > userdata->limit_global)
		verdict = 0;
	
	switch(verdict){
		case 0: cache_disable (backend); break;
		case 1: cache_enable  (backend); break;
	};
} // }}}

static int cache_init(backend_t *backend){ // {{{
	cache_userdata        *userdata = backend->userdata = calloc(1, sizeof(cache_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int cache_destroy(backend_t *backend){ // {{{
	cache_userdata        *userdata = (cache_userdata *)backend->userdata;
	
	cache_disable(backend);
	
	pthread_mutex_destroy(&userdata->lock);
	
	free(userdata);
	return 0;
} // }}}
static int cache_configure_any(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	pthread_mutexattr_t    attr; 
	uintmax_t              file_size;
	uintmax_t              cfg_limit_file    = __MAX(uintmax_t);
	uintmax_t              cfg_limit_fork    = __MAX(uintmax_t);
	uintmax_t              cfg_limit_global  = __MAX(uintmax_t);
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	pthread_mutexattr_init(&attr); 
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&userdata->lock, &attr);
	pthread_mutexattr_destroy(&attr);
	
	hash_data_copy(ret, TYPE_UINTT, cfg_limit_file,   config, HK(max_perfile));
	hash_data_copy(ret, TYPE_UINTT, cfg_limit_fork,   config, HK(max_perfork));
	hash_data_copy(ret, TYPE_UINTT, cfg_limit_global, config, HK(max_global));
	
	userdata->limit_perfile = cfg_limit_file;
	userdata->limit_perfork = cfg_limit_fork;
	userdata->limit_global  = cfg_limit_global;
	
	// try enable cache
	file_size = cache_get_filesize(backend);
	cache_check(backend, file_size);
	
	return 0;
} // }}}
static int cache_configure(backend_t *backend, config_t *config){ // {{{
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	// no fork, so use own data
	userdata->perfork_lock     = &userdata->lock;
	userdata->perfork_memusage = &userdata->memusage;
	
	return cache_configure_any(backend, config);
} // }}}
static int cache_fork(backend_t *backend, backend_t *parent, config_t *config){ // {{{
	cache_userdata        *userdata_parent   = (cache_userdata *)parent->userdata;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	
	// have fork, so use parent data
	userdata->perfork_lock     = &userdata_parent->lock;
	userdata->perfork_memusage = &userdata_parent->memusage;
	
	return cache_configure_any(backend, config);
} // }}}

static ssize_t cache_backend_read(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *buffer;
	data_ctx_t            *buffer_ctx;
	cache_userdata        *userdata = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 0)
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	
	return data_transfer(buffer, buffer_ctx, &d_memory, request);
} // }}}
static ssize_t cache_backend_write(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *buffer;
	data_ctx_t            *buffer_ctx;
	cache_userdata        *userdata = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 0)
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	
	return data_transfer(&d_memory, request, buffer, buffer_ctx);
} // }}}
static ssize_t cache_backend_create(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	size_t                 size;
	off_t                  offset;
	data_t                 offset_data       = DATA_PTR_OFFT(&offset);
	data_t                *offset_out;
	data_ctx_t            *offset_out_ctx;
	uintmax_t              file_size;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 0)
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	hash_data_copy(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return warning("no size supplied");
	
	pthread_mutex_lock(&userdata->lock);
		
		if(memory_grow(&userdata->memory, size, &offset) != 0){
			ret = error("memory_grow failed");
			goto end_grow;
		}
		
		if(memory_size(&userdata->memory, &file_size) != 0){
			ret = error("memory_size failed");
			goto end_grow;
		}
		
		cache_memusage (backend, DIR_INC, size, NULL, NULL);
		cache_check    (backend, file_size);
end_grow:
	pthread_mutex_unlock(&userdata->lock);
	
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
	
	pthread_mutex_lock(&userdata->lock);
		
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
		
		cache_memusage(backend, DIR_DEC, size, NULL, NULL);
exit:		
	pthread_mutex_unlock(&userdata->lock);
	
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

backend_t cache_proto = {
	.class          = "cache",
	.supported_api  = API_CRWD,
	.func_init      = &cache_init,
	.func_configure = &cache_configure,
	.func_fork      = &cache_fork,
	.func_destroy   = &cache_destroy,
	{{
		.func_create = &cache_backend_create,
		.func_get    = &cache_backend_read,
		.func_set    = &cache_backend_write,
		.func_delete = &cache_backend_delete,
		.func_count  = &cache_backend_count
	}}
};

