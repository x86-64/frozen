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
 * 	        class         = "cache",
 *              max_perfile   = (uint_t)'1000',    # set maximum memory usage for one instance to 1000 bytes
 *              max_perfork   = (uint_t)'2000',    # set maximum memory usage for group of forked backends to 2000 bytes
 *              max_global    = (uint_t)'3000',    # set maximum global memory usage (can be different for each instance)
 *              mode_perfork  =                    # limit apply mode:
 *                              "random",          #   - kill random item
 *                              "first",           #   - kill earliest registered item
 *                              "last",            #   - kill latest registered item
 *                              "highest_ticks",   #   - kill most usable backend
 *                              "lowest_ticks",    #   - kill lowest usable backend
 *                              "highest_mem",     #   - kill backend which occupy more memory
 *                              "lowest_mem",      #   - kill backend which occupy less memory
 *              mode_global   ... same as _perfork
 * 	}
 *
 * Memory limit behavior:
 *   - all write requests locked while flushing in\from memory to\from backend
 *   - _perfile checked on each create\delete request. Cache enable and disable performed in same request.
 *   - _perfork and _global limits checked ones per CACHE_FREE_INTERVAL (default 5 seconds). In between backends can overconsume
 *     memory.
 *   - ticks collected in CACHE_TICKS_INTERVAL period. _curr is current period ticks, _last - previous period ticks
 *
 * @endcode
 */

#define EMODULE 3
#define CACHE_PAGE_SIZE      0x1000
#define CACHE_FREE_INTERVAL  5
#define CACHE_TICKS_INTERVAL 5

typedef enum usage_dir {
	DIR_INC,
	DIR_DEC,
	DIR_NONE
} usage_dir;

typedef enum kill_method {
	METHOD_RANDOM,
	METHOD_FIRST,
	METHOD_LAST,
	METHOD_HIGHEST_TICKS,
	METHOD_LOWEST_TICKS,
	METHOD_HIGHEST_MEMORY,
	METHOD_LOWEST_MEMORY,
	
	METHOD_INVALID
} kill_method;

typedef struct cache_userdata {
	uintmax_t        enabled;
	memory_t         memory;
	
	// locks
	pthread_mutex_t  resize_lock;
	pthread_rwlock_t write_lock;
	
	// usage params
	uintmax_t        created_on;
	uintmax_t        usage_memory;
	uintmax_t        usage_ticks_last;  // 
	uintmax_t        usage_ticks_curr;  // no locks for this, coz we don't care about precise info
	uintmax_t        usage_ticks_time;  //
	
	// fork support
	list            *perfork_childs;
	list             perfork_childs_own;
	
	// limits
	uintmax_t        limit_perfile;
	uintmax_t        limit_perfork;
	uintmax_t        limit_global;
	kill_method      mode_perfork;
	kill_method      mode_global;
} cache_userdata;

static uintmax_t        running_caches          = 0;
static pthread_mutex_t  running_caches_mtx      = PTHREAD_MUTEX_INITIALIZER;
static pthread_t        main_thread;
static list             watch_perfork;
static list             watch_global;

static kill_method cache_string_to_method(char *string){ // {{{
	if(string != NULL){
		if(strcasecmp(string, "random") == 0)        return METHOD_RANDOM;
		if(strcasecmp(string, "first") == 0)         return METHOD_FIRST;
		if(strcasecmp(string, "last") == 0)          return METHOD_LAST;
		if(strcasecmp(string, "highest_ticks") == 0) return METHOD_HIGHEST_TICKS;
		if(strcasecmp(string, "lowest_ticks") == 0)  return METHOD_LOWEST_TICKS;
		if(strcasecmp(string, "highest_mem") == 0)   return METHOD_HIGHEST_MEMORY;
		if(strcasecmp(string, "lowest_mem") == 0)    return METHOD_LOWEST_MEMORY;
	}
	return METHOD_INVALID;
} // }}}
static inline void update_ticks(cache_userdata *userdata){ // {{{
	uintmax_t              time_curr;
	
	time_curr = time(NULL);
	if(userdata->usage_ticks_time + CACHE_TICKS_INTERVAL < time_curr){
		userdata->usage_ticks_time = time_curr;
		userdata->usage_ticks_last = userdata->usage_ticks_curr;
		userdata->usage_ticks_curr = 0;
	}
	userdata->usage_ticks_curr++;
} // }}}
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
		
		userdata->usage_memory += file_size;
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
		
		userdata->usage_memory -= file_size;
exit:
	pthread_mutex_unlock(&userdata->resize_lock);
} // }}}
void       cache_perform_limit(list *backends, kill_method method, uintmax_t limit, uintmax_t need_lock){
	uintmax_t              i, lsz;
	backend_t             *backend;
	cache_userdata        *backend_ud;
	backend_t            **lchilds;
	uintmax_t              usage_current;
	
	if(limit == __MAX(uintmax_t)) // no limits
		return;
	
	if(need_lock == 1) list_rdlock(backends);
		
		if( (lsz = list_count(backends)) != 0){
			lchilds = alloca( sizeof(backend_t *) * lsz );
			list_flatten(backends, (void **)lchilds, lsz);
			
repeat:;
			usage_current = 0;
			
			backend_t *backend_first         = NULL;
			backend_t *backend_last          = NULL;
			backend_t *backend_high_ticks    = NULL;
			backend_t *backend_low_ticks     = NULL;
			backend_t *backend_high_mem      = NULL;
			backend_t *backend_low_mem       = NULL;
			
			for(i=0; i<lsz; i++){
				backend    = lchilds[i];
				backend_ud = (cache_userdata *)backend->userdata;
				
				if(backend_ud->enabled == 0)
					continue;
				
				usage_current += backend_ud->usage_memory;
				
				#define cache_kill_check(_backend, _field, _moreless) { \
					if(_backend == NULL){ \
						_backend = backend; \
					}else{ \
						if(backend_ud->_field _moreless ((cache_userdata *)_backend->userdata)->_field) \
							_backend = backend; \
					} \
				}
				cache_kill_check(backend_first,      created_on,       <);
				cache_kill_check(backend_last,       created_on,       >);
				cache_kill_check(backend_low_ticks,  usage_ticks_last, <);
				cache_kill_check(backend_high_ticks, usage_ticks_last, >);
				cache_kill_check(backend_low_mem,    usage_memory,     <);
				cache_kill_check(backend_high_mem,   usage_memory,     >);
			}
			
			if(usage_current >= limit){
				switch(method){
					case METHOD_RANDOM:         backend = lchilds[ random() % lsz ]; break;
					case METHOD_FIRST:          backend = backend_first; break;
					case METHOD_LAST:           backend = backend_last; break;
					case METHOD_HIGHEST_TICKS:  backend = backend_high_ticks; break;
					case METHOD_LOWEST_TICKS:   backend = backend_low_ticks; break;
					case METHOD_HIGHEST_MEMORY: backend = backend_high_mem; break;
					case METHOD_LOWEST_MEMORY:  backend = backend_low_mem; break;
					default:                    backend = NULL; break;
				};
				if(backend != NULL){
					cache_disable(backend);
					goto repeat;
				}
			}
		}
		
	if(need_lock == 1) list_unlock(backends);
	
	return;
}
void *     cache_main_thread(void *null){ // {{{
	void                  *iter;
	backend_t             *backend;
	cache_userdata        *backend_ud;
	
	while(1){
		// preform global limits
		list_rdlock(&watch_global);
			iter = NULL;
			while( (backend = list_iter_next(&watch_global, &iter)) != NULL){
				backend_ud = (cache_userdata *)backend->userdata;
				
				cache_perform_limit(&watch_global, backend_ud->mode_global, backend_ud->limit_global, 0);
			}
		list_unlock(&watch_global);
		
		// preform perfork limits
		list_rdlock(&watch_perfork);
			iter = NULL;
			while( (backend = list_iter_next(&watch_perfork, &iter)) != NULL){
				backend_ud = (cache_userdata *)backend->userdata;
				
				cache_perform_limit(backend_ud->perfork_childs, backend_ud->mode_perfork, backend_ud->limit_perfork, 1);
			}
		list_unlock(&watch_perfork);
		
		sleep(CACHE_FREE_INTERVAL);
	};
	return NULL;
} // }}}

static int cache_init(backend_t *backend){ // {{{
	ssize_t                ret               = 0;
	cache_userdata        *userdata          = backend->userdata = calloc(1, sizeof(cache_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	// global
	pthread_mutex_lock(&running_caches_mtx);
		if(running_caches++ == 0){
			list_init(&watch_perfork);
			list_init(&watch_global);
			
			if(pthread_create(&main_thread, NULL, &cache_main_thread, NULL) != 0)
				ret = error("pthread_create failed");
		}
	pthread_mutex_unlock(&running_caches_mtx);
	
	return ret;
} // }}}
static int cache_destroy(backend_t *backend){ // {{{
	void                  *res;
	ssize_t                ret               = 0;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	// remove cache
	cache_disable(backend);
	pthread_mutex_destroy(&userdata->resize_lock);
	pthread_rwlock_destroy(&userdata->write_lock);
	
	if(userdata->perfork_childs == &userdata->perfork_childs_own){
		list_destroy(&userdata->perfork_childs_own);
	}else{
		list_delete(userdata->perfork_childs, backend);
	}
	list_delete(&watch_perfork, backend);
	list_delete(&watch_global, backend);
	free(userdata);
	
	// global
	pthread_mutex_lock(&running_caches_mtx);
		if(--running_caches == 0){
			if(pthread_cancel(main_thread) != 0){
				ret = error("pthread_cancel failed");
				goto exit;
			}
			if(pthread_join(main_thread, &res) != 0){
				ret = error("pthread_join failed");
				goto exit;
			}
			// TODO memleak here
			
			list_destroy(&watch_perfork);
			list_destroy(&watch_global);
		}
exit:
	pthread_mutex_unlock(&running_caches_mtx);
	
	return ret;
} // }}}
static int cache_configure_any(backend_t *backend, backend_t *parent, config_t *config){ // {{{
	ssize_t                ret;
	pthread_mutexattr_t    attr; 
	uintmax_t              file_size;
	uintmax_t              cfg_limit_file    = __MAX(uintmax_t);
	uintmax_t              cfg_limit_fork    = __MAX(uintmax_t);
	uintmax_t              cfg_limit_global  = __MAX(uintmax_t);
	char                  *cfg_mode_perfork  = "first";
	char                  *cfg_mode_global   = "first";
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, cfg_mode_perfork, config, HK(mode_perfork));
	if( (userdata->mode_perfork = cache_string_to_method(cfg_mode_perfork)) == METHOD_INVALID)
		return error("invalid mode_perfork supplied");
	
	hash_data_copy(ret, TYPE_STRINGT, cfg_mode_global,  config, HK(mode_global));
	if( (userdata->mode_global = cache_string_to_method(cfg_mode_global)) == METHOD_INVALID)
		return error("invalid mode_global supplied");
	
	pthread_mutexattr_init(&attr); 
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&userdata->resize_lock, &attr);
	pthread_mutexattr_destroy(&attr);
	
	pthread_rwlock_init(&userdata->write_lock, NULL);
	
	hash_data_copy(ret, TYPE_UINTT,   cfg_limit_file,   config, HK(max_perfile));
	hash_data_copy(ret, TYPE_UINTT,   cfg_limit_fork,   config, HK(max_perfork));
	if(ret != 0)
		list_add(&watch_perfork, (parent == NULL) ? backend : parent);
	
	hash_data_copy(ret, TYPE_UINTT,   cfg_limit_global, config, HK(max_global));
	if(ret != 0)
		list_add(&watch_global,  backend);
	
	userdata->limit_perfile = cfg_limit_file;
	userdata->limit_perfork = cfg_limit_fork;
	userdata->limit_global  = cfg_limit_global;
	userdata->created_on    = time(NULL);
	userdata->usage_memory  = 0;
	
	// try enable cache
	file_size = cache_get_filesize(backend);
	if(file_size <= userdata->limit_perfile)
		cache_enable(backend);
	
	return 0;
} // }}}
static int cache_configure(backend_t *backend, config_t *config){ // {{{
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	// no fork, so use own data
	userdata->perfork_childs       = &userdata->perfork_childs_own;
	list_init(&userdata->perfork_childs_own);
	
	return cache_configure_any(backend, NULL, config);
} // }}}
static int cache_fork(backend_t *backend, backend_t *parent, config_t *config){ // {{{
	cache_userdata        *userdata_parent   = (cache_userdata *)parent->userdata;
	cache_userdata        *userdata          = (cache_userdata *)backend->userdata;
	
	// have fork, so use parent data
	userdata->perfork_childs       = &userdata_parent->perfork_childs_own;
	list_add(userdata->perfork_childs, backend);
	
	return cache_configure_any(backend, parent, config);
} // }}}

static ssize_t cache_backend_read(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *buffer;
	data_ctx_t            *buffer_ctx;
	cache_userdata        *userdata = (cache_userdata *)backend->userdata;
	
	if(userdata->enabled == 0)
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	// update usage ticks
	update_ticks(userdata);
	
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
			// update usage ticks
			update_ticks(userdata);
			
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
	
	// update usage ticks
	update_ticks(userdata);
	
	hash_data_copy(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return warning("no size supplied");
	
	pthread_mutex_lock(&userdata->resize_lock);
		
		if(memory_grow(&userdata->memory, size, &offset) != 0){
			ret = error("memory_grow failed");
			goto end_grow;
		}
		
		if(memory_size(&userdata->memory, &userdata->usage_memory) != 0){
			ret = error("memory_size failed");
			goto end_grow;
		}
		
		if(userdata->usage_memory > userdata->limit_perfile)
			cache_disable(backend);
		
end_grow:
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
	
	// update usage ticks
	update_ticks(userdata);
	
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
		
		userdata->usage_memory = (uintmax_t)offset;
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

