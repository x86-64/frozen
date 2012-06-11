#include <libfrozen.h>
#include <container_t.h>

#include <core/void/void_t.h>
#include <io/io/io_t.h>
#include <storage/raw/raw_t.h>
#include <enum/format/format_t.h>
#include <modifiers/slider/slider_t.h>

typedef ssize_t (*container_callback)(data_t *data, uintmax_t offset, uintmax_t size, void *userdata);

typedef struct container_ctx {
	container_callback     callback;
	uintmax_t              start_offset;
	uintmax_t              size;
	uintmax_t              format;
	
	uintmax_t              curr_offset;
	void                  *userdata;
} container_ctx;

typedef struct container_convert_ctx {
	data_t                *sl_data;
	format_t               format;
	uintmax_t              size;
} container_convert_ctx;

typedef struct container_size_ctx {
	uintmax_t              size;
} container_size_ctx;

typedef struct container_io_ctx {
	void                  *buffer;
	uintmax_t              size;
	uintmax_t              called;
} container_io_ctx;

static uintmax_t         data_get_size(data_t *data, format_t format){ // {{{
	fastcall_length r_len = { { 4, ACTION_LENGTH }, 0, format };
	if(data_query(data, &r_len) < 0)
		return 0;
	
	return r_len.length;
} // }}}

static ssize_t container_iter(data_t *iot, container_ctx *ctx, fastcall_create *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              chunk_offset;
	uintmax_t              chunk_size;
	uintmax_t              call              = 1;
	data_t                *chunk_data        = fargs->value;
	
	if(ctx->size == 0 || fargs->value == NULL)
		return 0;
	
	chunk_size = data_get_size(chunk_data, ctx->format);
								
	if(
		ctx->start_offset >= ctx->curr_offset               &&
		ctx->start_offset <  ctx->curr_offset + chunk_size
	){
		chunk_offset = (ctx->start_offset - ctx->curr_offset);
	}else if(ctx->curr_offset > ctx->start_offset){
		chunk_offset = 0;
	}else{
		call = 0;
	}
	
	ctx->curr_offset += chunk_size;
	
	if(call == 1){
		chunk_size = chunk_size - chunk_offset;
		chunk_size = MIN(chunk_size, ctx->size);
		
		if( (ret = ctx->callback(chunk_data, chunk_offset, chunk_size, ctx->userdata)) != 0)
			return ret;
								
		ctx->size -= chunk_size;
	}
	return 0;
} // }}}
static ssize_t container_iter_convert_to(data_t *iot, container_convert_ctx *ctx, fastcall_create *fargs){ // {{{
	ssize_t                ret;
	data_t                *chunk_data        = fargs->value;
	
	if(fargs->value == NULL)
		return 0;
	
	fastcall_convert_to r_convert = {
		{ 5, ACTION_CONVERT_TO },
		ctx->sl_data,
		ctx->format
	};
	if( (ret = data_query(chunk_data, &r_convert)) < 0)
		return ret;
	
	data_slider_t_set_offset(ctx->sl_data, r_convert.transfered, SEEK_CUR);
	
	ctx->size += r_convert.transfered;
	return 0;
} // }}}
static ssize_t container_iter_convert_from(data_t *iot, container_convert_ctx *ctx, fastcall_create *fargs){ // {{{
	ssize_t                ret;
	data_t                *chunk_data        = fargs->value;
	
	if(fargs->value == NULL)
		return 0;
	
	fastcall_convert_from r_convert = {
		{ 5, ACTION_CONVERT_FROM },
		ctx->sl_data,
		ctx->format
	};
	if( (ret = data_query(chunk_data, &r_convert)) < 0)
		return ret;
	
	data_slider_t_set_offset(ctx->sl_data, r_convert.transfered, SEEK_CUR);
	
	ctx->size += r_convert.transfered;
	return 0;
} // }}}

static ssize_t container_iter_size(data_t *chunk_data, uintmax_t chunk_offset, uintmax_t chunk_size, container_size_ctx *ctx){ // {{{
	ctx->size += chunk_size;
	return 0;
} // }}}
static ssize_t container_iter_read(data_t *chunk_data, uintmax_t chunk_offset, uintmax_t chunk_size, container_io_ctx *ctx){ // {{{
	ssize_t                ret;
	
	ctx->called = 1;
	
	fastcall_read r_read = {
		{ 5, ACTION_READ },
		chunk_offset,
		ctx->buffer + ctx->size,
		chunk_size
	};
	if( (ret = data_query(chunk_data, &r_read)) < 0)
		return ret;
	
	ctx->size += r_read.buffer_size;
	return 0;
} // }}}
static ssize_t container_iter_write(data_t *chunk_data, uintmax_t chunk_offset, uintmax_t chunk_size, container_io_ctx *ctx){ // {{{
	ssize_t                ret;
	
	ctx->called = 1;
	
	fastcall_write r_write = {
		{ 5, ACTION_WRITE },
		chunk_offset,
		ctx->buffer + ctx->size,
		chunk_size
	};
	if( (ret = data_query(chunk_data, &r_write)) < 0)
		return ret;
	
	ctx->size += r_write.buffer_size;
	return 0;
} // }}}

ssize_t        data_container_t              (data_t *data, data_t storage){ // {{{
	container_t           *fdata;
	
	if( (fdata = malloc(sizeof(container_t))) == NULL)
		return -ENOMEM;
	
	fdata->storage = storage;
	
	data->type = TYPE_CONTAINERT;
	data->ptr  = fdata;
	return 0;
} // }}}
void           data_container_t_destroy      (data_t *data){ // {{{
	container_t           *fdata             = (container_t *)data->ptr;
	
	if(fdata){
		data_free(&fdata->storage);
		free(fdata);
	}
	data_set_void(data);
} // }}}

static ssize_t data_container_t_pass(data_t *data, fastcall_header *hargs){ // {{{
	container_t           *fdata             = (container_t *)data->ptr;
	
	return data_query(&fdata->storage, hargs);
} // }}}
static ssize_t data_container_t_len(data_t *data, fastcall_length *fargs){ // {{{
	ssize_t                ret;
	container_t           *fdata             = (container_t *)data->ptr;
	
	container_size_ctx size_ctx = {
		.size         = 0
	};
	container_ctx ctx = {
		.callback     = (container_callback)&container_iter_size,
		.start_offset = 0,
		.size         = ~0,
		.format       = fargs->format,
		.curr_offset  = 0,
		.userdata     = &size_ctx
	};
	data_t                 d_iot             = DATA_IOT((void *)&ctx, (f_io_func)&container_iter);
	fastcall_enum          r_enum            = {
		{ 3, ACTION_ENUM },
		&d_iot
	};
	if( (ret = data_query(&fdata->storage, &r_enum)) < 0)
		return ret;
	
	fargs->length = size_ctx.size;
	return 0;
} // }}}
static ssize_t data_container_t_read(data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret               = -1;
	container_t           *fdata             = (container_t *)data->ptr;
	
	container_io_ctx io_ctx = {
		.buffer       = fargs->buffer,
		.size         = 0,
		.called       = 0
	};
	container_ctx ctx = {
		.callback     = (container_callback)&container_iter_read,
		.start_offset = fargs->offset,
		.size         = fargs->buffer_size,
		.format       = FORMAT(native),
		.curr_offset  = 0,
		.userdata     = &io_ctx
	};
	data_t                 d_iot             = DATA_IOT((void *)&ctx, (f_io_func)&container_iter);
	fastcall_enum          r_enum            = {
		{ 3, ACTION_ENUM },
		&d_iot
	};
	if( (ret = data_query(&fdata->storage, &r_enum)) < 0)
		return ret;
	
	fargs->buffer_size = io_ctx.size;
	return (io_ctx.called == 1) ? 0 : -1;
} // }}}
static ssize_t data_container_t_write(data_t *data, fastcall_write *fargs){ // {{{
	ssize_t                ret               = -1;
	container_t           *fdata             = (container_t *)data->ptr;
	
	container_io_ctx io_ctx = {
		.buffer       = fargs->buffer,
		.size         = 0,
		.called       = 0
	};
	container_ctx ctx = {
		.callback     = (container_callback)&container_iter_write,
		.start_offset = fargs->offset,
		.size         = fargs->buffer_size,
		.format       = FORMAT(native),
		.curr_offset  = 0,
		.userdata     = &io_ctx
	};
	data_t                 d_iot             = DATA_IOT((void *)&ctx, (f_io_func)&container_iter);
	fastcall_enum          r_enum            = {
		{ 3, ACTION_ENUM },
		&d_iot
	};
	if( (ret = data_query(&fdata->storage, &r_enum)) < 0)
		return ret;
	
	fargs->buffer_size = io_ctx.size;
	return (io_ctx.called == 1) ? 0 : -1;
} // }}}
static ssize_t data_container_t_convert_to(data_t *data, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	container_t           *fdata             = (container_t *)data->ptr;
	
	data_t                 sl_data = DATA_SLIDERT(fargs->dest, 0);
	container_convert_ctx  convert_ctx = {
		.sl_data      = &sl_data,
		.size         = 0,
		.format       = fargs->format
	};
	data_t                 d_iot             = DATA_IOT((void *)&convert_ctx, (f_io_func)&container_iter_convert_to);
	fastcall_enum          r_enum            = {
		{ 3, ACTION_ENUM },
		&d_iot
	};
	if( (ret = data_query(&fdata->storage, &r_enum)) < 0)
		return ret;
	
	if(fargs->header.nargs >= 5)
		fargs->transfered = convert_ctx.size;
	
	return 0;
} // }}}
static ssize_t data_container_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	container_t           *fdata             = (container_t *)dst->ptr;
	
	if(fdata == NULL){
		switch(fargs->format){
			case FORMAT(hash):;
				data_t                 storage;
				
				holder_consume(ret, storage, fargs->src);
				if(ret < 0)
					return ret;
				
				return data_container_t(dst, storage);
			
			case FORMAT(native):;
				data_t                     fargs_src_storage;
				container_t               *fargs_src        = (container_t *)fargs->src->ptr;
				
				if(dst->type == fargs->src->type){
					holder_copy(ret, &fargs_src_storage, &fargs_src->storage);
					if(ret < 0)
						return ret;
					
					if( (ret = data_container_t(dst, fargs_src_storage)) < 0){
						data_free(&fargs_src_storage);
						return ret;
					}
					return 0;
				}
				
			default:
				break;
		}
	}else{
		data_t                 sl_src      = DATA_SLIDERT(fargs->src, 0);
		container_convert_ctx  convert_ctx = {
			.sl_data      = &sl_src,
			.size         = 0,
			.format       = fargs->format
		};
		data_t                 d_iot             = DATA_IOT((void *)&convert_ctx, (f_io_func)&container_iter_convert_from);
		fastcall_enum          r_enum            = {
			{ 3, ACTION_ENUM },
			&d_iot
		};
		if( (ret = data_query(&fdata->storage, &r_enum)) < 0)
			return ret;
		
		if(fargs->header.nargs >= 5)
			fargs->transfered = convert_ctx.size;
		
		return ret;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_container_t_free(data_t *data, fastcall_free *fargs){ // {{{
	data_container_t_destroy(data);
	return 0;
} // }}}
static ssize_t data_container_t_view(data_t *data, fastcall_view *fargs){ // {{{
	ssize_t                ret;
	data_t                 d_view         = DATA_RAWT_EMPTY();
	
	fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, &d_view, fargs->format };
	if( (ret = data_query(data, &r_convert)) < 0)
		return ret;
	
	fastcall_view       r_view    = { { 6, ACTION_VIEW }, FORMAT(native) };
	if( (ret = data_query(&d_view, &r_view)) < 0){
		data_free(&d_view);
		return ret;
	}
	
	fargs->ptr    = r_view.ptr;
	fargs->length = r_view.length;
	fargs->freeit = d_view;
	return 0;
} // }}}

data_proto_t container_t_proto = {
	.type                   = TYPE_CONTAINERT,
	.type_str               = "container_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handlers               = {
		[ACTION_LENGTH]       = (f_data_func)&data_container_t_len,
		[ACTION_READ]         = (f_data_func)&data_container_t_read,
		[ACTION_WRITE]        = (f_data_func)&data_container_t_write,
		[ACTION_FREE]         = (f_data_func)&data_container_t_free,
		[ACTION_CONVERT_TO]   = (f_data_func)&data_container_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_container_t_convert_from,
		
		[ACTION_LOOKUP]       = (f_data_func)&data_container_t_pass,
		[ACTION_ENUM]         = (f_data_func)&data_container_t_pass,
		[ACTION_VIEW]         = (f_data_func)&data_container_t_view,
	}
};
