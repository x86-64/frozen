#include <libfrozen.h>

typedef ssize_t (*f_convert_to_hash)  (void *, void *,      f_fast_to_hash);
typedef ssize_t (*f_convert_to_fast)  (void *, request_t *, f_hash_to_fast);

extern f_convert_to_hash   api_convert_to_hash   [ACTION_INVALID];
extern f_convert_to_fast   api_convert_to_fast   [ACTION_INVALID];
extern hash_t *            api_data_from_list    [ACTION_INVALID];

ssize_t api_machine_nosys(machine_t *machine, request_t *request){ // {{{
	return -ENOSYS;
} // }}}
ssize_t api_data_nosys(data_t *data, fastcall_header *hargs){ // {{{
	return -ENOSYS;
} // }}}

static ssize_t remove_symlinks(hash_t *hash_item, hash_t **new_hash){ // {{{
	ssize_t                ret;
	data_t                *data;
	hash_t                *new_hash_item;
	
	data_realholder(ret, &hash_item->data, data);
	if(ret < 0)
		return ITER_BREAK;
	
	if( (new_hash_item = hash_new(3)) == NULL)
		return ITER_BREAK;
	
	new_hash_item[0].key       = hash_item->key;
	new_hash_item[0].data.type = data->type;
	new_hash_item[0].data.ptr  = data->ptr;
	hash_assign_hash_inline(&new_hash_item[1], *new_hash);
	
	*new_hash = new_hash_item;
	return ITER_CONTINUE;
} // }}}

ssize_t     action_crud_to_hash(void *userdata, fastcall_crud *fargs, f_fast_to_hash callback){ // {{{
	ssize_t                ret;
	
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_null,
		hash_null,
		hash_end
	};
	if(fargs->key){
		r_next[1].key  = HK(key);
		r_next[1].data = *fargs->key;
	}
	if(fargs->value){
		r_next[2].key  = HK(value);
		r_next[2].data = *fargs->value;
	}
	ret = callback(userdata, r_next);
	
	if(fargs->key)
		*fargs->key   = r_next[1].data;
	if(fargs->value)
		*fargs->value = r_next[2].data;
	return ret;
} // }}}
ssize_t     action_crud_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	ssize_t                ret;
	action_t               action;
	data_t                *key;
	data_t                *value;
	
	hash_data_get(ret, TYPE_ACTIONT, action, request, HK(action));
	if(ret != 0)
		return ret;
	
	key   = hash_data_find(request, HK(key));
	value = hash_data_find(request, HK(value));
	
	if(key){
		data_realholder(ret, key, key);
		if(ret < 0)
			return ret;
	}
	if(value){
		data_realholder(ret, value, value);
		if(ret < 0)
			return ret;
	}
	
	fastcall_crud fargs = { { 4, action }, key, value };
	ret = callback(userdata, &fargs);
	
	return ret;
} // }}}

ssize_t     action_io_to_hash(void *userdata, fastcall_io *fargs, f_fast_to_hash callback){ // {{{
	ssize_t                ret;
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(offset),     DATA_UINTT(      fargs->offset                     ) },
		{ HK(buffer),     DATA_RAW(        fargs->buffer, fargs->buffer_size ) },
		{ HK(size),       DATA_UINTT(      fargs->buffer_size                ) },
		hash_end
	};
	ret = callback(userdata, r_next);

	fargs->offset = DEREF_TYPE_UINTT(&r_next[1].data);
	fargs->buffer_size = DEREF_TYPE_UINTT(&r_next[3].data);
	return ret;
} // }}}
ssize_t     action_read_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
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
	
	data_t                 d_slice    = DATA_SLICET(userdata, offset, size); // FIXME
	fastcall_convert_to      r_convert = { { 5, ACTION_CONVERT_TO }, r_buffer, FORMAT(native) };
	
	ret = data_query(&d_slice, &r_convert);
	
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &r_convert.transfered, sizeof(r_convert.transfered) };
	if(r_size)
		data_query(r_size, &r_write);
	
	return ret;
} // }}}
ssize_t     action_write_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
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
	
	data_t                 d_slice    = DATA_SLICET(userdata, offset, size); // FIXME
	fastcall_convert_to    r_convert = { { 5, ACTION_CONVERT_TO }, &d_slice, FORMAT(native) };
	
	ret = data_query(r_buffer, &r_convert);
	
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &r_convert.transfered, sizeof(r_convert.transfered) };
	if(r_size)
		data_query(r_size, &r_write);
	
	return ret;
} // }}}

ssize_t     action_resize_to_hash(void *userdata, fastcall_resize *fargs, f_fast_to_hash callback){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(size),       DATA_UINTT(      fargs->length                     ) },
		hash_end
	};
	return callback(userdata, r_next);
} // }}}
ssize_t     action_resize_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	ssize_t                ret;
	fastcall_resize        fargs             = { { 4, ACTION_RESIZE } };
	
	hash_data_get(ret, TYPE_UINTT, fargs.length, request, HK(size));
	
	return callback(userdata, &fargs);
} // }}}

ssize_t     action_push_to_hash(void *userdata, fastcall_push *fargs, f_fast_to_hash callback){ // {{{
	if(fargs->data == NULL)
		return -EINVAL;
	
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(data),       *fargs->data                                         },
		hash_end
	};
	return callback(userdata, r_next);
} // }}}
ssize_t     action_push_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	ssize_t                ret;
	data_t                *pointer;
	data_t                 holder;
	data_t                *holder_ptr        = NULL;
	
	if( (pointer = hash_data_find(request, HK(data))) != NULL){
		data_realholder(ret, pointer, pointer);
		if(ret < 0)
			return ret;
		
		holder_consume(ret, holder, pointer);
		if(ret != 0)
			return -EINVAL;
		
		if(holder.type != TYPE_VOIDT) // same as missing HK(data)
			holder_ptr = &holder;
	}
	
	fastcall_push          fargs             = { { 3, ACTION_PUSH }, holder_ptr };
	return callback(userdata, &fargs);
} // }}}

ssize_t     action_pop_to_hash(void *userdata, fastcall_pop *fargs, f_fast_to_hash callback){ // {{{
	if(fargs->data == NULL)
		return -EINVAL;
	
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(data),       *fargs->data                                         },
		hash_end
	};
	return callback(userdata, r_next);
} // }}}
ssize_t     action_pop_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	ssize_t                ret;
	data_t                *pointer;
	
	if( (pointer = hash_data_find(request, HK(data))) == NULL)
		return -EINVAL;
		
	data_realholder(ret, pointer, pointer);
	if(ret < 0)
		return ret;
	
	fastcall_pop           fargs             = { { 3, ACTION_POP }, pointer };
	return callback(userdata, &fargs);
} // }}}

ssize_t     action_free_to_hash(void *userdata, fastcall_free *fargs, f_fast_to_hash callback){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return callback(userdata, r_next);
} // }}}
ssize_t     action_free_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	fastcall_free           fargs             = { { 2, ACTION_FREE } };
	return callback(userdata, &fargs);
} // }}}

ssize_t     action_increment_to_hash(void *userdata, fastcall_increment *fargs, f_fast_to_hash callback){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return callback(userdata, r_next);
} // }}}
ssize_t     action_increment_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	fastcall_increment           fargs             = { { 2, ACTION_INCREMENT } };
	return callback(userdata, &fargs);
} // }}}

ssize_t     action_decrement_to_hash(void *userdata, fastcall_decrement *fargs, f_fast_to_hash callback){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		hash_end
	};
	return callback(userdata, r_next);
} // }}}
ssize_t     action_decrement_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	fastcall_decrement           fargs             = { { 2, ACTION_DECREMENT } };
	return callback(userdata, &fargs);
} // }}}

ssize_t     action_enum_to_hash(void *userdata, fastcall_enum *fargs, f_fast_to_hash callback){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(data),      *fargs->dest                                          },
		hash_end
	};
	return callback(userdata, r_next);
} // }}}
ssize_t     action_enum_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	data_t                *dest_data;
	
	if( (dest_data = hash_data_find(request, HK(data))) == NULL)
		return -EINVAL;
	
	fastcall_enum          fargs             = { { 4, ACTION_ENUM }, dest_data };
	return callback(userdata, &fargs);
} // }}}

ssize_t     action_length_to_hash(void *userdata, fastcall_length *fargs, f_fast_to_hash callback){ // {{{
	ssize_t                ret;
	format_t               format            = fargs->format;
	
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(size),       DATA_UINTT(      fargs->length                     ) },
		{ HK(format),     DATA_PTR_FORMATT( &format                          ) },
		hash_end
	};
	ret = callback(userdata, r_next);

	fargs->length = DEREF_TYPE_UINTT(&r_next[1].data);
	return ret;
} // }}}
ssize_t     action_length_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	ssize_t                ret;
	format_t               format            = FORMAT(native);
	
	hash_data_get(ret, TYPE_FORMATT, format, request, HK(format));
	
	fastcall_length        fargs             = { { 4, ACTION_LENGTH }, 0, format };
	ret = callback(userdata, &fargs);
	
	data_t                *buffer            = hash_data_find(request, HK(size));
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &fargs.length, sizeof(fargs.length) };
	if(buffer)
		data_query(buffer, &r_write);
	
	return ret;
} // }}}

ssize_t     action_query_to_hash(void *userdata, fastcall_query *fargs, f_fast_to_hash callback){ // {{{
	request_t  r_next[] = {
		{ HK(action),     DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(request),    DATA_PTR_HASHT( fargs->request                     ) },
		hash_end
	};
	return callback(userdata, r_next);
} // }}}
ssize_t     action_query_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	ssize_t                ret;
	request_t             *t, *n;
	request_t             *q_request;
	request_t             *q_request_clean   = NULL;
	
	hash_data_get(ret, TYPE_HASHT, q_request, request, HK(request));
	if(ret != 0)
		return -EINVAL;
	
	if(hash_iter(q_request, (hash_iterator)&remove_symlinks, &q_request_clean, 0) == ITER_OK){
		fastcall_query         fargs             = { { 3, ACTION_QUERY }, q_request_clean };
		ret = callback(userdata, &fargs);
	}else{
		ret = -EFAULT;
	}
	
	for(t = q_request_clean; t; t = n){
		n = t[1].data.ptr; // inline hash
		
		free(t);
	}
	return ret;
} // }}}
	
ssize_t     action_convert_to_to_hash(void *userdata, fastcall_convert_to *fargs, f_fast_to_hash callback){ // {{{
	ssize_t                ret;
	format_t               format            = fargs->format;
	
	request_t  r_next[] = {
		{ HK(action),      DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(destination), *fargs->dest                                         },
		{ HK(format),      DATA_PTR_FORMATT( &format                          ) },
		{ HK(size),        DATA_UINTT(      fargs->transfered                 ) },
		hash_end
	};
	ret = callback(userdata, r_next);

	fargs->transfered = DEREF_TYPE_UINTT(&r_next[3].data);
	return ret;
} // }}}
ssize_t     action_convert_to_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	ssize_t                ret;
	data_t                *output;
	format_t               format            = FORMAT(native);
	
	hash_data_get(ret, TYPE_FORMATT, format, request, HK(format));
	
	if( (output = hash_data_find(request, HK(destination))) == NULL)
		return -EINVAL;
	
	fastcall_convert_to    fargs             = { { 5, ACTION_CONVERT_TO }, output, format };
	ret = callback(userdata, &fargs);
	
	data_t                *buffer            = hash_data_find(request, HK(size));
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &fargs.transfered, sizeof(fargs.transfered) };
	if(buffer)
		data_query(buffer, &r_write);
	
	return ret;
} // }}}

ssize_t     action_convert_from_to_hash(void *userdata, fastcall_convert_from *fargs, f_fast_to_hash callback){ // {{{
	ssize_t                ret;
	format_t               format            = fargs->format;
	
	request_t  r_next[] = {
		{ HK(action),      DATA_PTR_ACTIONT( &fargs->header.action            ) },
		{ HK(source),     *fargs->src                                           },
		{ HK(format),      DATA_PTR_FORMATT( &format                          ) },
		{ HK(size),        DATA_UINTT(      fargs->transfered                 ) },
		hash_end
	};
	ret = callback(userdata, r_next);
	
	fargs->transfered = DEREF_TYPE_UINTT(&r_next[3].data);
	return ret;
} // }}}
ssize_t     action_convert_from_to_fast(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	ssize_t                ret;
	data_t                *input;
	format_t               format            = FORMAT(native);
	
	hash_data_get(ret, TYPE_FORMATT, format, request, HK(format));
	
	if( (input = hash_data_find(request, HK(source))) == NULL)
		return -EINVAL;
	
	fastcall_convert_from    fargs             = { { 5, ACTION_CONVERT_FROM }, input, format };
	ret = callback(userdata, &fargs);
	
	data_t                *buffer            = hash_data_find(request, HK(size));
	fastcall_write         r_write           = { { 5, ACTION_WRITE }, 0, &fargs.transfered, sizeof(fargs.transfered) };
	if(buffer)
		data_query(buffer, &r_write);
	
	return ret;
} // }}}

ssize_t     api_convert_fastcall_to_request(void *userdata, fastcall_header *hargs, f_fast_to_hash callback){ // {{{
	f_convert_to_hash    func;
	
	if(
		((fastcall_header *)hargs)->action >= ACTION_INVALID ||
		(func = api_convert_to_hash[ ((fastcall_header *)hargs)->action]) == NULL
	)
		return -ENOSYS;
	
	return func(userdata, hargs, callback);
} // }}}
ssize_t     api_convert_request_to_fastcall(void *userdata, request_t *request, f_hash_to_fast callback){ // {{{
	ssize_t                ret;
	action_t               action;
	f_convert_to_fast      func;
	
	hash_data_get(ret, TYPE_ACTIONT, action, request, HK(action));
	if(
		ret != 0 ||
		action >= ACTION_INVALID ||
		(func = api_convert_to_fast[action]) == NULL
	)
		return -ENOSYS;
	
	return func(userdata, request, callback);
} // }}}

static ssize_t api_pack_fastcall_callback(data_t *output, request_t *request){ // {{{
	data_t                 d_request         = DATA_PTR_HASHT(request);

	fastcall_convert_to r_convert_to = { { 4, ACTION_CONVERT_TO }, output, FORMAT(packed) };
	return data_query(&d_request, &r_convert_to);
} // }}}
static ssize_t api_unpack_fastcall_callback(fastcall_header *output, fastcall_header *input){ // {{{
	uintmax_t              header_size;
	
	header_size = MIN(input->nargs, output->nargs);
	
	if(header_size > FASTCALL_NARGS_LIMIT)      // too many arguments
		return -EFAULT;
	
	header_size *= sizeof(output->nargs);
	
	memcpy(output, input, header_size);
	return 0;
} // }}}
ssize_t     api_pack_fastcall(fastcall_header *input, data_t *output){ // {{{
	return api_convert_fastcall_to_request(output, input, (f_fast_to_hash)api_pack_fastcall_callback);
} // }}}
ssize_t     api_unpack_fastcall(data_t *input, fastcall_header *output){ // {{{
	ssize_t                ret;
	data_t                 d_request         = DATA_PTR_HASHT(NULL);
	
	fastcall_convert_from r_convert_from = { { 4, ACTION_CONVERT_FROM }, input, FORMAT(packed) };
	if( (ret = data_query(&d_request, &r_convert_from)) < 0)
		return ret;
	
	return api_convert_request_to_fastcall(output, data_get_ptr(&d_request), (f_hash_to_fast)api_unpack_fastcall_callback);
} // }}}

static ssize_t     data_query_callback(void *userdata, void *hargs){ // {{{
	return data_query( (data_t *)userdata, (fastcall_header *)hargs);
} // }}}
static ssize_t     machine_query_callback(void *userdata, request_t *request){ // {{{
	return machine_query( (machine_t *)userdata, request);
} // }}}
ssize_t     data_list_query(data_t *data, request_t *list){ // {{{
	ssize_t                ret;
	action_t               action;
	
	data_get(ret, TYPE_ACTIONT, action, &list->data);
	if(
		ret != 0 ||
		action >= ACTION_INVALID
	)
		return -ENOSYS;
	
	hash_rename(list, api_data_from_list[action]);
	
	return data_hash_query(data, list);
} // }}}
ssize_t     data_hash_query(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	
	data_realholder(ret, data, data);
	if(ret < 0)
		return ret;
	
	return api_convert_request_to_fastcall(data, request, data_query_callback);
} // }}}
ssize_t     machine_fast_query(machine_t *machine, void *hargs){ // {{{
	return api_convert_fastcall_to_request(machine, hargs, machine_query_callback);
} // }}}

uintmax_t fastcall_nargs[ACTION_INVALID] = {
	[ACTION_CREATE] = 4,
	[ACTION_READ] = 5,
	[ACTION_WRITE] = 5,
	[ACTION_DELETE] = 4,
	[ACTION_RESIZE] = 3,
	[ACTION_ENUM]  = 4,
	[ACTION_PUSH] = 3,
	[ACTION_POP] = 3,
	[ACTION_FREE] = 2,
	[ACTION_INCREMENT] = 2,
	[ACTION_DECREMENT] = 2,
	[ACTION_QUERY] = 3,
	[ACTION_CONVERT_TO] = 4,
	[ACTION_CONVERT_FROM] = 4,
	
	[ACTION_COMPARE] = 3,
	[ACTION_ADD] = 3,
	[ACTION_SUB] = 3,
	[ACTION_MULTIPLY] = 3,
	[ACTION_DIVIDE] = 3,
	[ACTION_VIEW] = 6,
	[ACTION_IS_NULL] = 3,
};

f_convert_to_hash  api_convert_to_hash[ACTION_INVALID] = {
	[ACTION_CREATE]       = (f_convert_to_hash)&action_crud_to_hash,
	[ACTION_LOOKUP]       = (f_convert_to_hash)&action_crud_to_hash,
	[ACTION_UPDATE]       = (f_convert_to_hash)&action_crud_to_hash,
	[ACTION_DELETE]       = (f_convert_to_hash)&action_crud_to_hash,
	[ACTION_READ]         = (f_convert_to_hash)&action_io_to_hash,
	[ACTION_WRITE]        = (f_convert_to_hash)&action_io_to_hash,
	[ACTION_RESIZE]       = (f_convert_to_hash)&action_resize_to_hash,
	[ACTION_ENUM]         = (f_convert_to_hash)&action_enum_to_hash,
	[ACTION_PUSH]         = (f_convert_to_hash)&action_push_to_hash,
	[ACTION_POP]          = (f_convert_to_hash)&action_pop_to_hash,
	[ACTION_FREE]         = (f_convert_to_hash)&action_free_to_hash,
	[ACTION_INCREMENT]    = (f_convert_to_hash)&action_increment_to_hash,
	[ACTION_DECREMENT]    = (f_convert_to_hash)&action_decrement_to_hash,
	[ACTION_LENGTH]       = (f_convert_to_hash)&action_length_to_hash,
	[ACTION_QUERY]        = (f_convert_to_hash)&action_query_to_hash,
	[ACTION_CONVERT_TO]   = (f_convert_to_hash)&action_convert_to_to_hash,
	[ACTION_CONVERT_FROM] = (f_convert_to_hash)&action_convert_from_to_hash,
};

f_convert_to_fast api_convert_to_fast[ACTION_INVALID] = {
	[ACTION_CREATE]       = (f_convert_to_fast)&action_crud_to_fast,
	[ACTION_LOOKUP]       = (f_convert_to_fast)&action_crud_to_fast,
	[ACTION_UPDATE]       = (f_convert_to_fast)&action_crud_to_fast,
	[ACTION_DELETE]       = (f_convert_to_fast)&action_crud_to_fast,
	[ACTION_READ]         = (f_convert_to_fast)&action_read_to_fast,
	[ACTION_WRITE]        = (f_convert_to_fast)&action_write_to_fast,
	[ACTION_RESIZE]       = (f_convert_to_fast)&action_resize_to_fast,
	[ACTION_ENUM]         = (f_convert_to_fast)&action_enum_to_fast,
	[ACTION_PUSH]         = (f_convert_to_fast)&action_push_to_fast,
	[ACTION_POP]          = (f_convert_to_fast)&action_pop_to_fast,
	[ACTION_FREE]         = (f_convert_to_fast)&action_free_to_fast,
	[ACTION_INCREMENT]    = (f_convert_to_fast)&action_increment_to_fast,
	[ACTION_DECREMENT]    = (f_convert_to_fast)&action_decrement_to_fast,
	[ACTION_LENGTH]       = (f_convert_to_fast)&action_length_to_fast,
	[ACTION_QUERY]        = (f_convert_to_fast)&action_query_to_fast,
	[ACTION_CONVERT_TO]   = (f_convert_to_fast)&action_convert_to_to_fast,
	[ACTION_CONVERT_FROM] = (f_convert_to_fast)&action_convert_from_to_fast,
};

hash_t * api_data_from_list[ACTION_INVALID] = {
	[ACTION_CREATE]       = (hash_t []){ { HK(action), DATA_VOID }, { HK(key),         DATA_VOID }, { HK(value),  DATA_VOID } },
	[ACTION_LOOKUP]       = (hash_t []){ { HK(action), DATA_VOID }, { HK(key),         DATA_VOID }, { HK(value),  DATA_VOID } },
	[ACTION_UPDATE]       = (hash_t []){ { HK(action), DATA_VOID }, { HK(key),         DATA_VOID }, { HK(value),  DATA_VOID } },
	[ACTION_DELETE]       = (hash_t []){ { HK(action), DATA_VOID }, { HK(key),         DATA_VOID }, { HK(value),  DATA_VOID } },
	[ACTION_READ]         = (hash_t []){ { HK(action), DATA_VOID }, { HK(offset),      DATA_VOID }, { HK(buffer), DATA_VOID }, { HK(size), DATA_VOID } },
	[ACTION_WRITE]        = (hash_t []){ { HK(action), DATA_VOID }, { HK(offset),      DATA_VOID }, { HK(buffer), DATA_VOID }, { HK(size), DATA_VOID } },
	[ACTION_RESIZE]       = (hash_t []){ { HK(action), DATA_VOID }, { HK(size),        DATA_VOID } },
	[ACTION_PUSH]         = (hash_t []){ { HK(action), DATA_VOID }, { HK(data),        DATA_VOID } },
	[ACTION_POP]          = (hash_t []){ { HK(action), DATA_VOID }, { HK(data),        DATA_VOID } },
	[ACTION_FREE]         = (hash_t []){ { HK(action), DATA_VOID } },
	[ACTION_INCREMENT]    = (hash_t []){ { HK(action), DATA_VOID } },
	[ACTION_DECREMENT]    = (hash_t []){ { HK(action), DATA_VOID } },
	[ACTION_ENUM]         = (hash_t []){ { HK(action), DATA_VOID }, { HK(data),        DATA_VOID } },
	[ACTION_LENGTH]       = (hash_t []){ { HK(action), DATA_VOID }, { HK(size),        DATA_VOID }, { HK(format), DATA_VOID }, },
	[ACTION_QUERY]        = (hash_t []){ { HK(action), DATA_VOID }, { HK(request),     DATA_VOID } },
	[ACTION_CONVERT_TO]   = (hash_t []){ { HK(action), DATA_VOID }, { HK(destination), DATA_VOID }, { HK(format), DATA_VOID }, { HK(size), DATA_VOID } },
	[ACTION_CONVERT_FROM] = (hash_t []){ { HK(action), DATA_VOID }, { HK(source),      DATA_VOID }, { HK(format), DATA_VOID }, { HK(size), DATA_VOID } },
};
