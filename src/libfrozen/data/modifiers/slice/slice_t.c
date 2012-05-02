#include <libfrozen.h>
#include <slice_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>
#include <core/default/default_t.h>
#include <storage/raw/raw_t.h>

static ssize_t       data_slice_t_handler  (data_t *data, fastcall_header *fargs){ // {{{
	slice_t               *fdata             = (slice_t *)data->ptr;
	
	return data_query(fdata->data, fargs);
} // }}}
static ssize_t       data_slice_t_len      (data_t *data, fastcall_length *fargs){ // {{{
	ssize_t                ret;
	slice_t               *fdata             = (slice_t *)data->ptr;
	
	if( (ret = data_query(fdata->data, fargs)) != 0)
		return ret;
	
	if( fdata->off > fargs->length ){
		fargs->length = 0;
		return 0;
	}
	
	fargs->length = MIN( fdata->size, fargs->length - fdata->off );
	return 0;
} // }}}
static ssize_t       data_slice_t_io       (data_t *data, fastcall_io *fargs){ // {{{
	slice_t               *fdata             = (slice_t *)data->ptr;
	
	if(fargs->offset > fdata->size)
		return -EINVAL;
	
	fargs->buffer_size  = MIN(fargs->buffer_size, fdata->size - fargs->offset);
	
	if(fargs->buffer_size == 0)
		return -1;
	
	fargs->offset      += fdata->off;
	return data_query(fdata->data, fargs);
} // }}}
static ssize_t       data_slice_t_convert_to    (data_t *src, fastcall_convert_to *fargs){ // {{{
	uintmax_t             *transfered;
	slice_t               *fdata             = (slice_t *)src->ptr;
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	transfered = (fargs->header.nargs >= 5) ? &fargs->transfered : NULL;
	
	return default_transfer(fdata->data, fargs->dest, fdata->off, 0, fdata->size, transfered);
} // }}}
static ssize_t       data_slice_t_convert_from    (data_t *dest, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	data_t                 input;
	hash_t                *config;
	slice_t               *fdata             = (slice_t *)dest->ptr;
	
	switch(fargs->format){
		case FORMAT(hash):;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if(fdata == NULL){
				if( (fdata = dest->ptr = malloc(sizeof(slice_t))) == NULL)
					return -ENOMEM;
				
				fdata->off  = 0;
				fdata->size = ~0;
			}
			
			hash_data_get(ret, TYPE_UINTT, fdata->off,    config, HK(offset));
			hash_data_get(ret, TYPE_UINTT, fdata->size,   config, HK(size));
			hash_holder_consume(ret, input, config, HK(data));
			if(ret != 0)
				return -EINVAL;
			
			if( (fdata->data = malloc(sizeof(data_t))) == NULL)
				return -ENOMEM;

			*fdata->data = input;
			return 0;
			
		default:                              // packed or native - create raw_t instead of slice_t
		                                      // (coz we can not pack slice_t into raw_t in convert_to)
			data_raw_t_empty(dest);

			return data_query(dest, fargs);
	}
	return -ENOSYS;
} // }}}
static ssize_t       data_slice_t_free(data_t *data, fastcall_free *fargs){ // {{{
	slice_t               *fdata             = (slice_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
	slice_t_free(fdata);
	return 0;
} // }}}
static ssize_t       data_slice_t_getdata(data_t *data, fastcall_getdata *fargs){ // {{{
	fargs->data = data;
	return 0;
} // }}}
static ssize_t       data_slice_t_getdataptr(data_t *data, fastcall_getdataptr *fargs){ // {{{
	return -ENOSYS;
} // }}}

data_proto_t slice_t_proto = {
	.type                   = TYPE_SLICET,
	.type_str               = "slice_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_GREEDY,
	.handler_default        = (f_data_func)&data_slice_t_handler,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_slice_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_slice_t_convert_from,
		[ACTION_READ]         = (f_data_func)&data_slice_t_io,
		[ACTION_WRITE]        = (f_data_func)&data_slice_t_io,
		[ACTION_LENGTH]       = (f_data_func)&data_slice_t_len,
		[ACTION_FREE]         = (f_data_func)&data_slice_t_free,
		[ACTION_GETDATA]      = (f_data_func)&data_slice_t_getdata,
		[ACTION_GETDATAPTR]   = (f_data_func)&data_slice_t_getdataptr,
	}
};

slice_t *       slice_t_alloc               (data_t *data, uintmax_t offset, uintmax_t size){ // {{{
	slice_t                *slice;

	if( (slice = malloc(sizeof(slice_t))) == NULL)
		return NULL;
	
	slice->data = data;
	slice->off  = offset;
	slice->size = size;
	return slice;
} // }}}
void            slice_t_free                (slice_t *slice){ // {{{
	free(slice);
} // }}}

