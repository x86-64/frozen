#include <libfrozen.h>
#include <machine.h>
#define EMODULE 4

typedef struct cacheapp_userdata {
	size_t      inited;
	memory_t    memory;
	machine_t  *machine;
	off_t       cache_start_offset;
	size_t      cache_max_size;
} cacheapp_userdata;

#define DEF_CACHE_SIZE 1000

// LIMIT no cross-file-memory reads and write 
/*
static ssize_t  cacheapp_flush(cacheapp_userdata *userdata){ // {{{
	ssize_t    ret;
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	data_t     d_machine  = DATA_MACHINET(userdata->machine);
	data_ctx_t d_machine_ctx[] = {
		{ HK(offset), DATA_PTR_OFFT(&userdata->cache_start_offset) },
		hash_end
	};
	ret = data_transfer(&d_machine, d_machine_ctx, &d_memory, NULL);
	
	// free cache
	memory_resize(&userdata->memory, 0);
	
	userdata->cache_start_offset += ret;
	return 0;
} // }}}
*/
static int cacheapp_init(machine_t *machine){ // {{{
	cacheapp_userdata *userdata = machine->userdata = calloc(1, sizeof(cacheapp_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int cacheapp_destroy(machine_t *machine){ // {{{
	cacheapp_userdata *userdata = (cacheapp_userdata *)machine->userdata;
	
	if(userdata->inited == 1){
		//cacheapp_flush(userdata);
		memory_free(&userdata->memory);
	}
	free(userdata);
	return 0;
} // }}}
static int cacheapp_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t             ret;
	uint64_t            file_size;
	size_t              cache_max_size = DEF_CACHE_SIZE;
	cacheapp_userdata  *userdata       = (cacheapp_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_SIZET, cache_max_size, config, HK(size));
	if(cache_max_size == 0)
		return error("size too small");
	
	//get file size //
	request_t r_count[] = {
		{ HK(action), DATA_ACTIONT(ACTION_COUNT)   },
		{ HK(buffer), DATA_PTR_UINT64T(&file_size)      },
		hash_end
	};
	if(machine_pass(machine, r_count) < 0)
		return error("count failed");
	
	if(__MAX(size_t) < file_size) // TODO add support and remove it
		return 0;
	
	userdata->cache_start_offset = file_size;
	userdata->cache_max_size     = cache_max_size;
	userdata->machine            = machine;
	
	memory_new(&userdata->memory, MEMORY_PAGE, ALLOC_MALLOC, 0x1000, 1);
	
	userdata->inited             = 1;
	return 0;
} // }}}
/*
static ssize_t cacheapp_machine_read(machine_t *machine, request_t *request){ // {{{
	ssize_t            ret;
	off_t              offset;
	data_t            *buffer;
	data_ctx_t        *buffer_ctx;
	cacheapp_userdata *userdata = (cacheapp_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_OFFT, offset, request, HK(offset));
	if(ret != 0 || offset < userdata->cache_start_offset)
		return ( (ret = machine_pass(machine, request)) < 0) ? ret : -EEXIST;
	
	offset -= userdata->cache_start_offset;
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	data_ctx_t d_memory_ctx[] = {
		{ HK(offset), DATA_PTR_OFFT(&offset) },
		hash_next(request)
	};
	
	return data_transfer(buffer, buffer_ctx, &d_memory, d_memory_ctx);
} // }}}
static ssize_t cacheapp_machine_write(machine_t *machine, request_t *request){ // {{{
	ssize_t            ret;
	off_t              offset;
	data_t            *buffer;
	data_ctx_t        *buffer_ctx;
	cacheapp_userdata *userdata = (cacheapp_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_OFFT, offset, request, HK(offset));
	if(ret != 0 || offset < userdata->cache_start_offset)
		return ( (ret = machine_pass(machine, request)) < 0) ? ret : -EEXIST;
	
	offset -= userdata->cache_start_offset;
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	data_t     d_memory = DATA_MEMORYT(&userdata->memory);
	data_ctx_t d_memory_ctx[] = {
		{ HK(offset), DATA_PTR_OFFT(&offset) },
		hash_next(request)
	};
	
	return data_transfer(&d_memory, d_memory_ctx, buffer, buffer_ctx);
} // }}}
static ssize_t cacheapp_machine_create(machine_t *machine, request_t *request){ // {{{
	ssize_t         ret;
	size_t          size;
	off_t           offset;
	data_t          offset_data = DATA_PTR_OFFT(&offset);
	data_t         *offset_out;
	data_ctx_t     *offset_out_ctx;
	cacheapp_userdata *userdata = (cacheapp_userdata *)machine->userdata;
	
	// check cache size
	if(memory_size(&userdata->memory, &size) < 0)
		return error("memory_size failed");
	if(size >= userdata->cache_max_size)
		cacheapp_flush(userdata);
	
	// do create
	hash_data_get(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return error("no size supplied");
	
	if(memory_grow(&userdata->memory, size, &offset) != 0)
		return error("memory_grow failed");
	
	//optional return of offset //
	offset += userdata->cache_start_offset;
	hash_data_find(request, HK(offset_out), &offset_out, &offset_out_ctx);
	data_transfer(offset_out, offset_out_ctx, &offset_data, NULL);
	
	//optional write from buffer //
	request_t r_write[] = {
		{ HK(offset), offset_data },
		hash_next(request)
	};
	if( (ret = cacheapp_machine_write(machine, r_write)) != -EINVAL)
		return ret;
	
	return 0;
} // }}}
static ssize_t cacheapp_machine_rest(machine_t *machine, request_t *request){ // {{{
	ssize_t            ret;
	off_t              file_size;
	cacheapp_userdata *userdata = (cacheapp_userdata *)machine->userdata;
	cacheapp_flush(userdata);
	
	if( (ret = machine_pass(machine, request)) == 0) ret = -EEXIST;
	
	//get file size //
	request_t r_count[] = {
		{ HK(action), DATA_ACTIONT(ACTION_COUNT)   },
		{ HK(buffer), DATA_PTR_UINT64T(&file_size)      },
		hash_end
	};
	if(machine_pass(machine, r_count) < 0)
		return error("count failed");
	
	if(__MAX(size_t) < file_size) // TODO add support and remove it
		return 0;
	
	userdata->cache_start_offset = file_size;
	return ret;
} // }}}
static ssize_t cacheapp_machine_count(machine_t *machine, request_t *request){ // {{{
	data_t         *buffer;
	data_ctx_t     *buffer_ctx;
	uint64_t        full_size;
	size_t          size;
	cacheapp_userdata *userdata = (cacheapp_userdata *)machine->userdata;
	
	if(memory_size(&userdata->memory, &size) < 0)
		return error("memory_size failed");
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	
	full_size = size + userdata->cache_start_offset;
	
	data_t count = DATA_PTR_UINT64T(&full_size);
	return data_transfer(
		buffer, buffer_ctx,
		&count, NULL
	);
} // }}}
*/
machine_t cache_append_proto = {
	.class          = "cache/cache-append",
	.supported_api  = API_CRWD,
	.func_init      = &cacheapp_init,
	.func_configure = &cacheapp_configure,
	.func_destroy   = &cacheapp_destroy,
	{
//		.func_create = &cacheapp_machine_create,
//		.func_get    = &cacheapp_machine_read,
//		.func_set    = &cacheapp_machine_write,
//		.func_delete = &cacheapp_machine_rest,
//		.func_move   = &cacheapp_machine_rest,
//		.func_count  = &cacheapp_machine_count
	}
};


