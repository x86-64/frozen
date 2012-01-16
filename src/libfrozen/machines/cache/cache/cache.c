#include <libfrozen.h>
#include <machine.h>
#include <pthread.h>

/**
 * @ingroup machine
 * @addtogroup mod_cache cache/cache
 *
 * Cache module hold data in memory, instead of underlying machine
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

static void cache_enable  (machine_t *machine);
static void cache_disable (machine_t *machine);

static size_t  cache_fast_create (machine_t *machine, off_t *offset, size_t size){ // {{{
	size_t                 ret               = size;
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	pthread_rwlock_wrlock(&userdata->lock);
		
		if(memory_grow(&userdata->memory, size, offset) != 0)
			ret = 0;
		
	pthread_rwlock_unlock(&userdata->lock);
	return ret;
} // }}}
static size_t  cache_fast_delete (machine_t *machine, off_t offset, size_t size){ // {{{
	uintmax_t              mem_size;
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
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
static size_t  cache_fast_read   (machine_t *machine, off_t offset, void *buffer, size_t size){ // {{{
	size_t                 ret;
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	pthread_rwlock_rdlock(&userdata->lock);
		
		ret = memory_read( &userdata->memory, offset, buffer, size);
	
	pthread_rwlock_unlock(&userdata->lock);
	return ret;
} // }}}
static size_t  cache_fast_write  (machine_t *machine, off_t offset, void *buffer, size_t size){ // {{{
	size_t                 ret;
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	pthread_rwlock_rdlock(&userdata->lock);
		
		ret = memory_write( &userdata->memory, offset, buffer, size);
	
	pthread_rwlock_unlock(&userdata->lock);
	return ret;
} // }}}
*/

static ssize_t cache_machine_read(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret, retd;
	uintmax_t              offset            = 0;
	uintmax_t              size              = ~0;
	data_t                *r_offset;
	data_t                *r_size;
	data_t                *r_buffer;
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	r_offset = hash_data_find(request, HK(offset));
	r_size   = hash_data_find(request, HK(size));
	r_buffer = hash_data_find(request, HK(buffer));
	
	data_get(retd, TYPE_UINTT, size,   r_size);
	data_get(retd, TYPE_UINTT, offset, r_offset);
	
	if(r_buffer == NULL)
		return -EINVAL;
	
	pthread_rwlock_rdlock(&userdata->lock);
	
		data_t d_slice = DATA_SLICET(&userdata->d_memory, offset, size);
		
		fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, r_buffer };
		ret = data_query(&d_slice, &r_transfer);
	
	pthread_rwlock_unlock(&userdata->lock);
	
	data_set(retd, TYPE_UINTT, r_transfer.transfered, r_size);
	return ret;
} // }}}
static ssize_t cache_machine_write(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret, retd;
	uintmax_t              offset            = 0;
	uintmax_t              size              = ~0;
	data_t                *r_offset;
	data_t                *r_size;
	data_t                *r_buffer;
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	r_offset = hash_data_find(request, HK(offset));
	r_size   = hash_data_find(request, HK(size));
	r_buffer = hash_data_find(request, HK(buffer));
	
	data_get(retd, TYPE_UINTT, size,   r_size);
	data_get(retd, TYPE_UINTT, offset, r_offset);
	
	if(r_buffer == NULL)
		return -EINVAL;
	
	pthread_rwlock_rdlock(&userdata->lock); // read lock, many requests allowed
		
		data_t d_slice = DATA_SLICET(&userdata->d_memory, offset, size);
		
		fastcall_transfer r_transfer = { { 4, ACTION_TRANSFER }, &d_slice };
		ret = data_query(r_buffer, &r_transfer);
	
	pthread_rwlock_unlock(&userdata->lock);
	
	data_set(retd, TYPE_UINTT, r_transfer.transfered, r_size);
	return ret;
} // }}}
static ssize_t cache_machine_create(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	size_t                 size;
	off_t                  offset;
	data_t                 offset_data       = DATA_PTR_OFFT(&offset);
	data_t                *offset_out;
	
	hash_data_copy(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return warning("no size supplied");
	
	if( (cache_fast_create(machine, &offset, size) != size) )
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
	if( (ret = cache_machine_write(machine, r_write)) != -EINVAL)
		return ret;
	
	return 0;
} // }}}
static ssize_t cache_machine_delete(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	off_t                  offset;
	size_t                 size;
	
	// TIP cache, as like as file, can only truncate
	
	hash_data_copy(ret, TYPE_OFFT,  offset, request, HK(offset));  if(ret != 0) return warning("offset not supplied");
	hash_data_copy(ret, TYPE_SIZET, size,   request, HK(size));    if(ret != 0) return warning("size not supplied");
	
	if( cache_fast_delete(machine, offset, size) != size )
		return error("cache delete failed");
	
	return 0;
} // }}}
static ssize_t cache_machine_count(machine_t *machine, request_t *request){ // {{{
	data_t                *buffer;
	uintmax_t              size;
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	pthread_rwlock_rdlock(&userdata->lock);
	
		if(memory_size(&userdata->memory, &size) < 0)
			return error("memory_size failed");
	
	pthread_rwlock_unlock(&userdata->lock);
	
	data_t count = DATA_PTR_UINTT(&size);
	
	buffer = hash_data_find(request, HK(buffer));
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, buffer };
	return data_query(&count, &r_transfer);
} // }}}

static uintmax_t cache_get_filesize(machine_t *machine){ // {{{
	ssize_t                ret;
	uintmax_t              file_size = 0;
	
	/* get file size */
	request_t r_count[] = {
		{ HK(action), DATA_UINT32T(ACTION_COUNT)   },
		{ HK(buffer), DATA_PTR_UINTT(&file_size)        },
		{ HK(ret),    DATA_PTR_SIZET(&ret)              },
		hash_end
	};
	if(machine_pass(machine, r_count) < 0 || ret < 0)
		return 0;
	
	return file_size;
} // }}}
static void      cache_enable(machine_t *machine){ // {{{
	uintmax_t              file_size;
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	pthread_rwlock_wrlock(&userdata->lock);
		
		if(userdata->enabled == 1)
			goto exit;
		
		file_size = cache_get_filesize(machine);
		
		// alloc new memory
		memory_new(&userdata->memory, MEMORY_PAGE, ALLOC_MALLOC, CACHE_PAGE_SIZE, 1);
		if(memory_resize(&userdata->memory, file_size) != 0)
			goto failed;
		
		// read data from machine to new memory
		data_t d_memory   = DATA_MEMORYT(&userdata->memory);
		data_t d_machine  = DATA_NEXT_MACHINET(machine);
		
		fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_memory };
		if(data_query(&d_machine, &r_transfer) < 0)
			goto failed;
		
		userdata->d_memory = d_memory;
		
		// enable caching
		userdata->enabled = 1;
		machine->machine_type_crwd.func_create      = &cache_machine_create;
		machine->machine_type_crwd.func_get         = &cache_machine_read;
		machine->machine_type_crwd.func_set         = &cache_machine_write;
		machine->machine_type_crwd.func_delete      = &cache_machine_delete;
		machine->machine_type_crwd.func_count       = &cache_machine_count;
		
exit:
	pthread_rwlock_unlock(&userdata->lock);
	return;
failed:
	memory_free(&userdata->memory);
	goto exit;
} // }}}
static void      cache_disable(machine_t *machine){ // {{{
	uintmax_t              file_size;
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	pthread_rwlock_wrlock(&userdata->lock);
	
		if(userdata->enabled == 0)
			goto exit;
		
		if(memory_size(&userdata->memory, &file_size) < 0)
			goto exit;
		
		// flush data from memory to machine
		data_t d_memory  = DATA_MEMORYT(&userdata->memory);
		data_t d_machine = DATA_NEXT_MACHINET(machine);
			
		fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_machine };
		data_query(&d_memory, &r_transfer);

		// free used memory
		memory_free(&userdata->memory);
		
		// disable caching
		userdata->enabled = 0;
		machine->machine_type_crwd.func_create      = NULL;
		machine->machine_type_crwd.func_get         = NULL;
		machine->machine_type_crwd.func_set         = NULL;
		machine->machine_type_crwd.func_delete      = NULL;
		machine->machine_type_crwd.func_count       = NULL;
		
exit:
	pthread_rwlock_unlock(&userdata->lock);
} // }}}

static int cache_init(machine_t *machine){ // {{{
	cache_userdata        *userdata          = machine->userdata = calloc(1, sizeof(cache_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int cache_destroy(machine_t *machine){ // {{{
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	cache_disable(machine);
	pthread_rwlock_destroy(&userdata->lock);
	
	free(userdata);
	return 0;
} // }}}
static int cache_configure(machine_t *machine, config_t *config){ // {{{
	cache_userdata        *userdata          = (cache_userdata *)machine->userdata;
	
	pthread_rwlock_init(&userdata->lock, NULL);
	cache_enable(machine);                               // try enable cache
	
	return 0;
} // }}}

machine_t cache_proto = {
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

