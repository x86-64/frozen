#include <libfrozen.h>
#include <slice_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>
#include <storage/raw/raw_t.h>
#include <io/io/io_t.h>

typedef struct enum_userdata {
	data_t                *data;
	fastcall_enum         *fargs;
	uintmax_t              skip_keys;
	uintmax_t              limit_keys;
} enum_userdata;

static ssize_t       data_slice_t_iter_iot(data_t *data, enum_userdata *userdata, fastcall_create *fargs){ // {{{
	if(fargs->value == NULL)        // skip end of enum
		return 0;
	
	if(userdata->skip_keys > 0){
		userdata->skip_keys--;
		return 0;               // skip key
	}
	
	if(userdata->limit_keys != ~(uintmax_t)0){
		if(userdata->limit_keys == 0)
			return -1;      // EOF
		
		userdata->limit_keys--;
	}
	
	fastcall_create r_create = { { 4, ACTION_CREATE }, fargs->key, fargs->value };
	return data_query(userdata->fargs->dest, &r_create);
} // }}}

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
static ssize_t       data_slice_t_view(data_t *data, fastcall_view *fargs){ // {{{
	ssize_t                ret;
	slice_t               *fdata             = (slice_t *)data->ptr;
	
	if( (ret = data_query(fdata->data, fargs)) < 0)
		return ret;
	
	if(fdata->off > fargs->length){
		ret = -EINVAL;
		goto error;
	}
	
	fargs->ptr    += fdata->off;
	fargs->length  = MIN(fdata->size, fargs->length - fdata->off);
	return 0;
	
error:
	data_free(&fargs->freeit);
	return ret;
} // }}}
static ssize_t       data_slice_t_control(data_t *data, fastcall_control *fargs){ // {{{
	slice_t               *fdata             = (slice_t *)data->ptr;
	
	switch(fargs->function){
		case HK(data):;
			helper_control_data_t r_internal[] = {
				{ HK(data), fdata->data      },
				{ 0, NULL }
			};
			return helper_control_data(data, fargs, r_internal);
			
		default:
			break;
	}
	return data_default_control(data, fargs);
} // }}}
static ssize_t       data_slice_t_lookup(data_t *data, fastcall_lookup *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              key;
	slice_t               *fdata             = (slice_t *)data->ptr;
	
	data_get(ret, TYPE_UINTT, key, fargs->key);
	if(ret < 0)
		return data_query(fdata->data, fargs);
	
	key += fdata->off;
	
	data_t                 d_key             = DATA_UINTT(key);
	fastcall_lookup r_lookup = { { 4, ACTION_LOOKUP }, &d_key, fargs->value };
	return data_query(fdata->data, &r_lookup);
} // }}}
static ssize_t       data_slice_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	slice_t               *fdata             = (slice_t *)data->ptr;
	
	enum_userdata          userdata          = { data, fargs, fdata->off, fdata->size };
	data_t                 iot               = DATA_IOT(&userdata, (f_io_func)&data_slice_t_iter_iot);
	fastcall_enum          r_enum            = { { 3, ACTION_ENUM }, &iot };
	ret = data_query(fdata->data, &r_enum);
	
	// last
	fastcall_create r_end = { { 3, ACTION_CREATE }, NULL, NULL };
	data_query(fargs->dest, &r_end);
	
	return (ret == -1) ? 0 : ret; // EOF can be returned in iter_iot function, this is fine behavior
} // }}}

data_proto_t slice_t_proto = {
	.type                   = TYPE_SLICET,
	.type_str               = "slice_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_GREEDY | DATA_PROXY,
	.handler_default        = (f_data_func)&data_slice_t_handler,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_slice_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_slice_t_convert_from,
		[ACTION_READ]         = (f_data_func)&data_slice_t_io,
		[ACTION_WRITE]        = (f_data_func)&data_slice_t_io,
		[ACTION_LENGTH]       = (f_data_func)&data_slice_t_len,
		[ACTION_FREE]         = (f_data_func)&data_slice_t_free,
		[ACTION_VIEW]         = (f_data_func)&data_slice_t_view,
		[ACTION_CONTROL]      = (f_data_func)&data_slice_t_control,
		[ACTION_LOOKUP]       = (f_data_func)&data_slice_t_lookup,
		[ACTION_ENUM]         = (f_data_func)&data_slice_t_enum,
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

