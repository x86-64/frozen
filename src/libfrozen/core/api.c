#include <libfrozen.h>

typedef ssize_t (*f_machine_from_fast)  (machine_t *, void *);
typedef ssize_t (*f_data_from_hash)     (data_t *, request_t *);

extern f_machine_from_fast api_machine_from_fast [ACTION_INVALID];
extern f_data_from_hash    api_data_from_hash    [ACTION_INVALID];

ssize_t api_machine_nosys(machine_t *machine, request_t *request){ // {{{
	return -ENOSYS;
} // }}}
ssize_t api_data_nosys(data_t *data, fastcall_header *hargs){ // {{{
	return -ENOSYS;
} // }}}

static ssize_t remove_symlinks(hash_t *hash_item, hash_t **new_hash){ // {{{
	hash_t                *new_hash_item;
	
	fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
	if( data_query(&hash_item->data, &r_getdata) < 0)
		return ITER_BREAK;
	
	if( (new_hash_item = hash_new(3)) == NULL)
		return ITER_BREAK;
	
	new_hash_item[0].key       = hash_item->key;
	new_hash_item[0].data.type = r_getdata.data->type;
	new_hash_item[0].data.ptr  = r_getdata.data->ptr;
	hash_assign_hash_inline(&new_hash_item[1], *new_hash);
	
	*new_hash = new_hash_item;
	return ITER_CONTINUE;
} // }}}

ssize_t     action_create_from_fast(machine_t *machine, fastcall_create *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(size),       DATA_PTR_UINTT( &fargs->size                       ) },
		{ HK(offset),     DATA_PTR_UINTT( &fargs->offset                     ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_create_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	fastcall_create        fargs             = { { 4, ACTION_CREATE } };
	
	hash_data_get(ret, TYPE_UINTT, fargs.size,   request, HK(size));
	
	ret = data_query(data, &fargs);
	
	data_t                *offset            = hash_data_find(request, HK(offset));
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &fargs.offset, sizeof(fargs.offset) };
	if(offset)
		data_query(offset, &r_write);
	
	return ret;
} // }}}

ssize_t     action_io_from_fast(machine_t *machine, fastcall_io *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(offset),     DATA_PTR_UINTT( &fargs->offset                     ) },
		{ HK(buffer),     DATA_RAW(        fargs->buffer, fargs->buffer_size ) },
		{ HK(size),       DATA_PTR_UINTT( &fargs->buffer_size                ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_read_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *r_buffer;
	data_t                *r_size;
	uintmax_t              offset            = 0;
	uintmax_t              size              = ~0;
	
	hash_data_get(ret, TYPE_UINTT, offset, request, HK(offset));
	
	if( (r_size   = hash_data_find(request, HK(size))) != NULL){
		data_get(ret, TYPE_UINTT, size, r_size);
	}
	
	if( (r_buffer = hash_data_find(request, HK(buffer))) == NULL)
		return -EINVAL;
	
	data_t                 d_slice    = DATA_SLICET(data, offset, size);
	fastcall_transfer      r_transfer = { { 4, ACTION_TRANSFER }, r_buffer };
	
	ret = data_query(&d_slice, &r_transfer);
	
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &r_transfer.transfered, sizeof(r_transfer.transfered) };
	if(r_size)
		data_query(r_size, &r_write);
	
	return ret;
} // }}}
ssize_t     action_write_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *r_buffer;
	data_t                *r_size;
	uintmax_t              offset            = 0;
	uintmax_t              size              = ~0;
	
	hash_data_get(ret, TYPE_UINTT, offset, request, HK(offset));
	
	if( (r_size   = hash_data_find(request, HK(size))) != NULL){
		data_get(ret, TYPE_UINTT, size, r_size);
	}
	
	if( (r_buffer = hash_data_find(request, HK(buffer))) == NULL)
		return -EINVAL;
	
	data_t                 d_slice    = DATA_SLICET(data, offset, size);
	fastcall_transfer      r_transfer = { { 4, ACTION_TRANSFER }, &d_slice };
	
	ret = data_query(r_buffer, &r_transfer);
	
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &r_transfer.transfered, sizeof(r_transfer.transfered) };
	if(r_size)
		data_query(r_size, &r_write);
	
	return ret;
} // }}}

ssize_t     action_delete_from_fast(machine_t *machine, fastcall_delete *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(offset),     DATA_PTR_UINTT( &fargs->offset                     ) },
		{ HK(size),       DATA_PTR_UINTT( &fargs->size                       ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_delete_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	fastcall_delete        fargs             = { { 4, ACTION_DELETE } };
	
	hash_data_get(ret, TYPE_UINTT, fargs.offset,   request, HK(offset));
	hash_data_get(ret, TYPE_UINTT, fargs.size,     request, HK(size));
	
	return data_query(data, &fargs);
} // }}}

ssize_t     action_count_from_fast(machine_t *machine, fastcall_count *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(buffer),     DATA_PTR_UINTT( &fargs->nelements                  ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_count_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	fastcall_count         fargs             = { { 3, ACTION_COUNT } };
	
	ret = data_query(data, &fargs);
	
	data_t                *buffer            = hash_data_find(request, HK(buffer));
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &fargs.nelements, sizeof(fargs.nelements) };
	if(buffer)
		data_query(buffer, &r_write);
	
	return ret;
} // }}}

ssize_t     action_transfer_from_fast(machine_t *machine, fastcall_transfer *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),      DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(destination), *fargs->dest                                         },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_transfer_from_hash(data_t *data, request_t *request){ // {{{
	data_t                *destination;
	
	if( (destination = hash_data_find(request, HK(destination))) == NULL)
		return -EINVAL;
	
	fastcall_transfer      fargs             = { { 3, ACTION_TRANSFER }, destination };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_resize_from_fast(machine_t *machine, fastcall_resize *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(size),       DATA_PTR_UINTT( &fargs->length                     ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_resize_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	fastcall_resize        fargs             = { { 4, ACTION_RESIZE } };
	
	hash_data_get(ret, TYPE_UINTT, fargs.length, request, HK(size));
	
	return data_query(data, &fargs);
} // }}}

ssize_t     action_push_from_fast(machine_t *machine, fastcall_push *fargs){ // {{{
	if(fargs->data == NULL)
		return -EINVAL;
	
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(data),       *fargs->data                                         },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_push_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *pointer;
	data_t                 holder;
	data_t                *holder_ptr        = NULL;
	
	if( (pointer = hash_data_find(request, HK(data))) != NULL){
		fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
		if( (ret = data_query(pointer, &r_getdata)) < 0)
			return ret;
		
		holder_consume(ret, holder, r_getdata.data);
		if(ret != 0)
			return -EINVAL;
		
		if(holder.type != TYPE_VOIDT) // same as missing HK(data)
			holder_ptr = &holder;
	}
	
	fastcall_push          fargs             = { { 3, ACTION_PUSH }, holder_ptr };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_pop_from_fast(machine_t *machine, fastcall_pop *fargs){ // {{{
	if(fargs->data == NULL)
		return -EINVAL;
	
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(data),       *fargs->data                                         },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_pop_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *pointer;
	
	if( (pointer = hash_data_find(request, HK(data))) == NULL)
		return -EINVAL;
	
	fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
	if( (ret = data_query(pointer, &r_getdata)) < 0)
		return ret;
	
	fastcall_pop           fargs             = { { 3, ACTION_POP }, r_getdata.data };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_start_from_fast(machine_t *machine, fastcall_start *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_start_from_hash(data_t *data, request_t *request){ // {{{
	fastcall_start           fargs             = { { 2, ACTION_START } };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_stop_from_fast(machine_t *machine, fastcall_stop *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_stop_from_hash(data_t *data, request_t *request){ // {{{
	fastcall_stop           fargs             = { { 2, ACTION_STOP } };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_free_from_fast(machine_t *machine, fastcall_free *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_free_from_hash(data_t *data, request_t *request){ // {{{
	fastcall_free           fargs             = { { 2, ACTION_FREE } };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_acquire_from_fast(machine_t *machine, fastcall_acquire *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_acquire_from_hash(data_t *data, request_t *request){ // {{{
	fastcall_acquire           fargs             = { { 2, ACTION_ACQUIRE } };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_increment_from_fast(machine_t *machine, fastcall_increment *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_increment_from_hash(data_t *data, request_t *request){ // {{{
	fastcall_increment           fargs             = { { 2, ACTION_INCREMENT } };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_decrement_from_fast(machine_t *machine, fastcall_decrement *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_decrement_from_hash(data_t *data, request_t *request){ // {{{
	fastcall_decrement           fargs             = { { 2, ACTION_DECREMENT } };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_execute_from_fast(machine_t *machine, fastcall_execute *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_execute_from_hash(data_t *data, request_t *request){ // {{{
	fastcall_execute           fargs             = { { 2, ACTION_EXECUTE } };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_copy_from_fast(machine_t *machine, fastcall_copy *fargs){ // {{{
	if(fargs->dest == NULL)
		return -EINVAL;
	
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(data),       *fargs->dest                                         },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_copy_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *pointer;
	
	if( (pointer = hash_data_find(request, HK(data))) == NULL)
		return -EINVAL;
	
	fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
	if( (ret = data_query(pointer, &r_getdata)) < 0)
		return ret;
	
	fastcall_copy           fargs             = { { 3, ACTION_COPY }, r_getdata.data };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_alloc_from_fast(machine_t *machine, fastcall_alloc *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(size),       DATA_PTR_UINTT( &fargs->length                     ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_alloc_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	fastcall_alloc         fargs             = { { 4, ACTION_ALLOC } };
	
	hash_data_get(ret, TYPE_UINTT, fargs.length, request, HK(size));
	
	return data_query(data, &fargs);
} // }}}

ssize_t     action_enum_from_fast(machine_t *machine, fastcall_enum *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(data),      *fargs->dest                                          },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_enum_from_hash(data_t *data, request_t *request){ // {{{
	data_t                *dest_data;
	
	if( (dest_data = hash_data_find(request, HK(data))) == NULL)
		return -EINVAL;
	
	fastcall_enum          fargs             = { { 4, ACTION_ENUM }, dest_data };
	return data_query(data, &fargs);
} // }}}

ssize_t     action_length_from_fast(machine_t *machine, fastcall_length *fargs){ // {{{
	format_t               format            = fargs->format;
	
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(size),       DATA_PTR_UINTT( &fargs->length                     ) },
		{ HK(format),     DATA_PTR_FORMATT( &format                          ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_length_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	format_t               format            = FORMAT(native);
	
	hash_data_get(ret, TYPE_FORMATT, format, request, HK(format));
	
	fastcall_length        fargs             = { { 4, ACTION_LENGTH }, 0, format };
	ret = data_query(data, &fargs);
	
	data_t                *buffer            = hash_data_find(request, HK(size));
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &fargs.length, sizeof(fargs.length) };
	if(buffer)
		data_query(buffer, &r_write);
	
	return ret;
} // }}}

ssize_t     action_query_from_fast(machine_t *machine, fastcall_query *fargs){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(request),    DATA_PTR_HASHT( fargs->request                     ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_query_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	request_t             *t, *n;
	request_t             *q_request;
	request_t             *q_request_clean   = NULL;
	
	hash_data_get(ret, TYPE_HASHT, q_request, request, HK(request));
	if(ret != 0)
		return -EINVAL;
	
	if(hash_iter(q_request, (hash_iterator)&remove_symlinks, &q_request_clean, 0) == ITER_OK){
		fastcall_query         fargs             = { { 3, ACTION_QUERY }, q_request_clean };
		ret = data_query(data, &fargs);
	}else{
		ret = -EFAULT;
	}
	
	for(t = q_request_clean; t; t = n){
		n = t[1].data.ptr; // inline hash
		
		free(t);
	}
	return ret;
} // }}}
	
ssize_t     action_convert_to_from_fast(machine_t *machine, fastcall_convert_to *fargs){ // {{{
	format_t               format            = fargs->format;
	
	request_t  r_next[] = {
		{ HK(action),      DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(destination), *fargs->dest                                         },
		{ HK(format),      DATA_PTR_FORMATT( &format                          ) },
		{ HK(size),        DATA_PTR_UINTT( &fargs->transfered                 ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_convert_to_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *output;
	format_t               format            = FORMAT(native);
	
	hash_data_get(ret, TYPE_FORMATT, format, request, HK(format));
	
	if( (output = hash_data_find(request, HK(destination))) == NULL)
		return -EINVAL;
	
	fastcall_convert_to    fargs             = { { 4, ACTION_CONVERT_TO }, output, format };
	ret = data_query(data, &fargs);
	
	data_t                *buffer            = hash_data_find(request, HK(size));
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &fargs.transfered, sizeof(fargs.transfered) };
	if(buffer)
		data_query(buffer, &r_write);
	
	return ret;
} // }}}

ssize_t     action_convert_from_from_fast(machine_t *machine, fastcall_convert_from *fargs){ // {{{
	format_t               format            = fargs->format;
	
	request_t  r_next[] = {
		{ HK(action),      DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(source),     *fargs->src                                           },
		{ HK(format),      DATA_PTR_FORMATT( &format                          ) },
		hash_end
	};
	return machine_query(machine, r_next);
} // }}}
ssize_t     action_convert_from_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *input;
	format_t               format            = FORMAT(native);
	
	hash_data_get(ret, TYPE_FORMATT, format, request, HK(format));
	
	if( (input = hash_data_find(request, HK(source))) == NULL)
		return -EINVAL;
	
	fastcall_convert_from    fargs             = { { 4, ACTION_CONVERT_FROM }, input, format };
	return data_query(data, &fargs);
} // }}}

ssize_t     data_hash_query(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	action_t               action;
	f_data_from_hash       func;
	
	hash_data_get(ret, TYPE_ACTIONT, action, request, HK(action));
	if(
		ret != 0 ||
		action >= ACTION_INVALID ||
		(func = api_data_from_hash[action]) == NULL
	)
		return -ENOSYS;
	
	fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
	if( (ret = data_query(data, &r_getdata)) < 0)
		return ret;
	
	return func(r_getdata.data, request);
} // }}}
ssize_t     machine_fast_query(machine_t *machine, void *hargs){ // {{{
	f_machine_from_fast    func;
	
	if(
		((fastcall_header *)hargs)->action >= ACTION_INVALID ||
		(func = api_machine_from_fast[ ((fastcall_header *)hargs)->action]) == NULL
	)
		return -ENOSYS;
	
	return func(machine, hargs);
} // }}}

uintmax_t fastcall_nargs[ACTION_INVALID] = {
	[ACTION_CREATE] = 4,
	[ACTION_READ] = 5,
	[ACTION_WRITE] = 5,
	[ACTION_DELETE] = 4,
	[ACTION_COUNT] = 3,
	[ACTION_TRANSFER] = 3,
	[ACTION_RESIZE] = 3,
	[ACTION_ENUM]  = 4,
	[ACTION_PUSH] = 3,
	[ACTION_POP] = 3,
	[ACTION_START] = 2,
	[ACTION_STOP] = 2,
	[ACTION_FREE] = 2,
	[ACTION_INCREMENT] = 2,
	[ACTION_DECREMENT] = 2,
	[ACTION_EXECUTE] = 2,
	[ACTION_COPY] = 3,
	[ACTION_ALLOC] = 3,
	[ACTION_QUERY] = 3,
	[ACTION_CONVERT_TO] = 4,
	[ACTION_CONVERT_FROM] = 4,
	[ACTION_ACQUIRE] = 2,
	
	[ACTION_COMPARE] = 3,
	[ACTION_ADD] = 3,
	[ACTION_SUB] = 3,
	[ACTION_MULTIPLY] = 3,
	[ACTION_DIVIDE] = 3,
	[ACTION_GETDATAPTR] = 3,
	[ACTION_IS_NULL] = 3,
};

f_machine_from_fast  api_machine_from_fast[ACTION_INVALID] = {
	[ACTION_CREATE]       = (f_machine_from_fast)&action_create_from_fast,
	[ACTION_READ]         = (f_machine_from_fast)&action_io_from_fast,
	[ACTION_WRITE]        = (f_machine_from_fast)&action_io_from_fast,
	[ACTION_DELETE]       = (f_machine_from_fast)&action_delete_from_fast,
	[ACTION_COUNT]        = (f_machine_from_fast)&action_count_from_fast,
	[ACTION_TRANSFER]     = (f_machine_from_fast)&action_transfer_from_fast,
	[ACTION_RESIZE]       = (f_machine_from_fast)&action_resize_from_fast,
	[ACTION_ENUM]         = (f_machine_from_fast)&action_enum_from_fast,
	[ACTION_PUSH]         = (f_machine_from_fast)&action_push_from_fast,
	[ACTION_POP]          = (f_machine_from_fast)&action_pop_from_fast,
	[ACTION_START]        = (f_machine_from_fast)&action_start_from_fast,
	[ACTION_STOP]         = (f_machine_from_fast)&action_stop_from_fast,
	[ACTION_FREE]         = (f_machine_from_fast)&action_free_from_fast,
	[ACTION_INCREMENT]    = (f_machine_from_fast)&action_increment_from_fast,
	[ACTION_DECREMENT]    = (f_machine_from_fast)&action_decrement_from_fast,
	[ACTION_EXECUTE]      = (f_machine_from_fast)&action_execute_from_fast,
	[ACTION_COPY]         = (f_machine_from_fast)&action_copy_from_fast,
	[ACTION_ALLOC]        = (f_machine_from_fast)&action_alloc_from_fast,
	[ACTION_LENGTH]       = (f_machine_from_fast)&action_length_from_fast,
	[ACTION_QUERY]        = (f_machine_from_fast)&action_query_from_fast,
	[ACTION_CONVERT_TO]   = (f_machine_from_fast)&action_convert_to_from_fast,
	[ACTION_CONVERT_FROM] = (f_machine_from_fast)&action_convert_from_from_fast,
	[ACTION_ACQUIRE]      = (f_machine_from_fast)&action_acquire_from_fast,
};

f_data_from_hash api_data_from_hash[ACTION_INVALID] = {
	[ACTION_CREATE]       = (f_data_from_hash)&action_create_from_hash,
	[ACTION_READ]         = (f_data_from_hash)&action_read_from_hash,
	[ACTION_WRITE]        = (f_data_from_hash)&action_write_from_hash,
	[ACTION_DELETE]       = (f_data_from_hash)&action_delete_from_hash,
	[ACTION_COUNT]        = (f_data_from_hash)&action_count_from_hash,
	[ACTION_TRANSFER]     = (f_data_from_hash)&action_transfer_from_hash,
	[ACTION_RESIZE]       = (f_data_from_hash)&action_resize_from_hash,
	[ACTION_ENUM]         = (f_data_from_hash)&action_enum_from_hash,
	[ACTION_PUSH]         = (f_data_from_hash)&action_push_from_hash,
	[ACTION_POP]          = (f_data_from_hash)&action_pop_from_hash,
	[ACTION_START]        = (f_data_from_hash)&action_start_from_hash,
	[ACTION_STOP]         = (f_data_from_hash)&action_stop_from_hash,
	[ACTION_FREE]         = (f_data_from_hash)&action_free_from_hash,
	[ACTION_INCREMENT]    = (f_data_from_hash)&action_increment_from_hash,
	[ACTION_DECREMENT]    = (f_data_from_hash)&action_decrement_from_hash,
	[ACTION_EXECUTE]      = (f_data_from_hash)&action_execute_from_hash,
	[ACTION_COPY]         = (f_data_from_hash)&action_copy_from_hash,
	[ACTION_ALLOC]        = (f_data_from_hash)&action_alloc_from_hash,
	[ACTION_LENGTH]       = (f_data_from_hash)&action_length_from_hash,
	[ACTION_QUERY]        = (f_data_from_hash)&action_query_from_hash,
	[ACTION_CONVERT_TO]   = (f_data_from_hash)&action_convert_to_from_hash,
	[ACTION_CONVERT_FROM] = (f_data_from_hash)&action_convert_from_from_hash,
	[ACTION_ACQUIRE]      = (f_data_from_hash)&action_acquire_from_hash,
};

