#include <libfrozen.h>
#define EMODULE 7
	/*
typedef struct allocator_userdata {
	hash_key_t     key;
	hash_key_t     key_out;
	hash_key_t     key_to;
	hash_key_t     key_from;
	hash_key_t     count;
	hash_key_t     size;
	DT_SIZET       multiply_as_sizet;
	DT_OFFT	multiply_as_offt;
	data_t	 multiply_as_sizet_data;
	data_t	 multiply_as_offt_data;
} allocator_userdata;
	*/

static int allocator_init(backend_t *backend){ // {{{
	//if((backend->userdata = calloc(1, sizeof(allocator_userdata))) == NULL)
	//	return error("calloc failed");
	
	return 0;
} // }}}
static int allocator_destroy(backend_t *backend){ // {{{
	//allocator_userdata *userdata = (allocator_userdata *)backend->userdata;
	//free(userdata);
	
	return 0;
} // }}}
static int allocator_configure(backend_t *backend, hash_t *config){ // {{{
	return 0;
} // }}}

static ssize_t allocator_backend_delete(backend_t *backend, request_t *request){
	ssize_t   ret;
	off_t     offset;
	size_t    size, curr_size;
	char      fill_buffer[255];
	
	hash_data_copy(ret, TYPE_OFFT,  offset,  request, HK(offset)); if(ret != 0) return warning("no offset supplied");
	hash_data_copy(ret, TYPE_SIZET, size,    request, HK(size)); if(ret != 0) return warning("no size supplied");
	
	while(size > 0){
		curr_size = MIN(size, 255);
		memset(fill_buffer, curr_size, curr_size);
		
		request_t r_delete[] = {
			{ HK(action), DATA_UINT32T(ACTION_CRWD_WRITE)     },
			{ HK(offset), DATA_PTR_OFFT(&offset)	        },
			{ HK(buffer), DATA_RAW(&fill_buffer, curr_size) },
			{ HK(size),   DATA_PTR_SIZET(&curr_size)	},
			hash_next(request)
		};
		if( (ret = backend_pass(backend, r_delete)) < 0)
			return ret;
		
		offset += curr_size;
		size   -= curr_size;
	}
	return 0;
}

static ssize_t allocator_backend_custom(backend_t *backend, request_t *request){
	ssize_t  ret;
	char    *function;
	
	hash_data_copy(ret, TYPE_STRINGT, function, request, HK(function));
	if(ret != 0)
		return warning("no function supplied");
	
	if(strcmp(function, "resize") == 0){
		off_t       rec_old_offset, rec_new_offset;
		size_t      rec_old_size,   rec_new_size;
		data_t      rec_new_offset_data = DATA_PTR_OFFT(&rec_new_offset);
		data_t     *offset_out;
		data_ctx_t *offset_out_ctx;
		
		hash_data_copy(ret, TYPE_OFFT,  rec_old_offset, request, HK(offset)); if(ret != 0) return warning("no offset supplied");
		hash_data_copy(ret, TYPE_SIZET, rec_new_size,   request, HK(size)); if(ret != 0) return warning("no size supplied");
		hash_data_copy(ret, TYPE_SIZET, rec_old_size,   request, HK(size_old)); if(ret != 0) return warning("no size_old supplied");
		
		// create new chunk
		rec_new_offset = 0; // TODO remove all inits
		request_t r_create[] = {
			{ HK(action),      DATA_UINT32T(ACTION_CRWD_CREATE) },
			{ HK(offset_out),  DATA_PTR_OFFT(&rec_new_offset) }, // TODO pass out new offset
			hash_next(request)
		};
		if( (ret = backend_pass(backend, r_create)) < 0)
			return ret;
		
		// move info from old to new one
		request_t r_move[] = {
			{ HK(action),      DATA_UINT32T(ACTION_CRWD_MOVE)   },
			{ HK(offset_from), DATA_PTR_OFFT(&rec_old_offset) },
			{ HK(offset_to),   DATA_PTR_OFFT(&rec_new_offset) },
			{ HK(size),        DATA_PTR_SIZET(&rec_old_size)  },
			hash_next(request)
		};
		if( (ret = backend_pass(backend, r_move)) < 0)
			return ret;
		
		// delete old
		request_t r_delete[] = {
			{ HK(action),      DATA_UINT32T(ACTION_CRWD_DELETE) },
			{ HK(size),        DATA_PTR_SIZET(&rec_old_size)  },
			hash_next(request)
		};
		if( (ret = backend_pass(backend, r_delete)) < 0)
			return ret;
		
		// optional write from buffer
		request_t r_write[] = {
			{ HK(offset),      DATA_PTR_OFFT(&rec_new_offset) },
			hash_next(request)
		};
		if( (ret = backend_pass(backend, r_write)) != -EINVAL)
			return ret;
		
		// optional offset fill
		hash_data_find(request, HK(offset_out), &offset_out, &offset_out_ctx);
		data_transfer(offset_out, offset_out_ctx, &rec_new_offset_data, NULL);
		return 0;
	}
	return warning("no such function");
}

backend_t allocator_proto = {
	.class          ="allocator",
	.supported_api  = API_CRWD,
	.func_init      = &allocator_init,
	.func_configure = &allocator_configure,
	.func_destroy   = &allocator_destroy,
	{{
		.func_delete  = &allocator_backend_delete,
		.func_custom  = &allocator_backend_custom
	}}
};


