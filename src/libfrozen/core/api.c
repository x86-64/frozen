#include <libfrozen.h>

typedef ssize_t (*f_machine_from_fast)  (machine_t *, void *);
typedef ssize_t (*f_data_from_hash)     (data_t *, request_t *);

extern f_machine_from_fast api_machine_from_fast [ACTION_INVALID];
extern f_data_from_hash    api_data_from_hash    [ACTION_INVALID];

typedef struct enum_ctx {
	machine_t             *shop;
	request_t             *request;
} enum_ctx;

static ssize_t action_enum_callback(request_t *request, enum_ctx *ctx){ // {{{
	request_t r_next[] = {
		hash_inline(request),
		hash_inline(ctx->request),
		hash_end
	};
	return machine_query(ctx->shop, r_next);
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

ssize_t     action_enum_from_hash(data_t *data, request_t *request){ // {{{
	ssize_t                ret;
	machine_t             *shop;
	
	hash_data_get(ret, TYPE_MACHINET, shop, request, HK(shop));
	
	enum_ctx               ctx               = { shop, request };
	fastcall_enum          fargs             = { { 4, ACTION_ENUM }, (f_callback)&action_enum_callback, &ctx };
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
	
	return func(data, request);
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
	[ACTION_READ] = 5,
	[ACTION_WRITE] = 5,
	[ACTION_PHYSICALLEN] = 3,
	[ACTION_LOGICALLEN] = 3,
	[ACTION_COPY] = 3,
	[ACTION_CONVERT_TO] = 4,
	[ACTION_CONVERT_FROM] = 4,
	[ACTION_COMPARE] = 3,
	[ACTION_ALLOC] = 3,
	[ACTION_FREE] = 2,
	[ACTION_ADD] = 3,
	[ACTION_SUB] = 3,
	[ACTION_MULTIPLY] = 3,
	[ACTION_DIVIDE] = 3,
	[ACTION_INCREMENT] = 2,
	[ACTION_DECREMENT] = 2,
	[ACTION_TRANSFER]  = 3,
	[ACTION_GETDATAPTR] = 3,
	[ACTION_IS_NULL] = 3,
	[ACTION_CREATE] = 4,
	[ACTION_DELETE] = 4,
	[ACTION_COUNT] = 3,
	[ACTION_EXECUTE] = 2,
	[ACTION_START] = 2,
	[ACTION_STOP] = 2,
	[ACTION_PUSH] = 3,
	[ACTION_POP] = 3,
	[ACTION_RESIZE] = 3,
	[ACTION_QUERY] = 3,
	[ACTION_ENUM]  = 4,
};

f_machine_from_fast  api_machine_from_fast[ACTION_INVALID] = {
	[ACTION_CREATE]       = (f_machine_from_fast)&action_create_from_fast,
	[ACTION_READ]         = (f_machine_from_fast)&action_io_from_fast,
	[ACTION_WRITE]        = (f_machine_from_fast)&action_io_from_fast,
	[ACTION_DELETE]       = (f_machine_from_fast)&action_delete_from_fast,
	[ACTION_COUNT]        = (f_machine_from_fast)&action_count_from_fast,
	[ACTION_TRANSFER]     = (f_machine_from_fast)&action_transfer_from_fast,
	[ACTION_RESIZE]       = (f_machine_from_fast)&action_resize_from_fast,
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
};

