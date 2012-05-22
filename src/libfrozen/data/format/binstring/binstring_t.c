#include <libfrozen.h>
#include <binstring_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>
#include <modifiers/slider/slider_t.h>
#include <modifiers/slice/slice_t.h>

#define BINSTRING_SIZE uint32_t

ssize_t data_binstring_t(data_t *data, data_t storage){ // {{{
	binstring_t           *fdata;
	
	if( (fdata = malloc(sizeof(binstring_t))) == NULL )
		return -ENOMEM;
	
	fdata->data       = &fdata->storage;
	fdata->storage    = storage;

	data->type = TYPE_BINSTRINGT;
	data->ptr  = fdata;
	return 0;
} // }}}
static ssize_t data_binstring_t_from_config(data_t *data, config_t *config){ // {{{
	ssize_t                ret;
	data_t                 storage;
	
	hash_holder_consume(ret, storage, config, HK(data));
	if(ret < 0)
		goto error1;
	
	if( (ret = data_binstring_t(data, storage)) < 0)
		goto error2;
	
	return 0;
	
error2:
	data_free(&storage);
error1:
	return ret;
} // }}}
static void    data_binstring_t_destroy(data_t *data){ // {{{
	binstring_t           *fdata             = (binstring_t *)data->ptr;
	
	if(fdata){
		data_free(&fdata->storage);
		free(fdata);
	}
	data_set_void(data);
} // }}}

static ssize_t data_binstring_t_handler (data_t *data, fastcall_header *fargs){ // {{{
	binstring_t               *fdata             = (binstring_t *)data->ptr;
	
	return data_query(fdata->data, fargs);
} // }}}
static ssize_t data_binstring_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	BINSTRING_SIZE         length;
	uintmax_t              transfered        = 0;
	binstring_t           *fdata             = (binstring_t *)src->ptr;
	data_t                 sl_dest           = DATA_SLIDERT(fargs->dest, 0);
	
	switch(fargs->format){
		case FORMAT(packed):;
		case FORMAT(binstring):;
			fastcall_length r_len = { { 3, ACTION_LENGTH }, 0, FORMAT(native) };
			if( (ret = data_query(fdata->data, &r_len)) < 0)
				return ret;
			
			length = r_len.length;
			
			// <size>
			fastcall_write r_write1 = { { 5, ACTION_WRITE }, 0, &length, sizeof(length) };
			ret         = data_query(&sl_dest, &r_write1);
			transfered += r_write1.buffer_size;
			data_slider_t_set_offset(&sl_dest, r_write1.buffer_size, SEEK_CUR);
			
			if(ret < 0)
				break;
			
			// <data>
			fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, &sl_dest, FORMAT(native) };
			ret         = data_query(fdata->data, &r_convert);
			transfered += r_convert.transfered;
			break;
			
		default:
			return data_query(fdata->data, fargs);
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_binstring_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	BINSTRING_SIZE         length;
	binstring_t           *fdata             = (binstring_t *)dst->ptr;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			if(fdata != NULL)          // we already inited - pass request
				break;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return data_binstring_t_from_config(dst, config);

		case FORMAT(binstring):
		case FORMAT(packed):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &length, sizeof(length) };
			if( (ret = data_query(fargs->src, &r_read)) < 0)
				return ret;
			
			data_t d_slice = DATA_SLICET(fargs->src, sizeof(length), length);
			
			fargs->src = &d_slice;
			ret = data_query(fdata->data, fargs);
			
			fargs->transfered += sizeof(length);
			return ret;
		
		case FORMAT(native):;
			data_t                     fargs_src_storage;
			binstring_t               *fargs_src        = (binstring_t *)fargs->src->ptr;
			
			if(fdata != NULL) // we already inited - pass
				break;
			
			if(dst->type != fargs->src->type)
				return -EINVAL;
			
			holder_copy(ret, &fargs_src_storage, &fargs_src->storage);
			if(ret < 0)
				return ret;
			
			if( (ret = data_binstring_t(dst, fargs_src_storage)) < 0){
				data_free(&fargs_src_storage);
				return ret;
			}
			return 0;
		
		default:
			break;
	}
	
	return data_binstring_t_handler(dst, (fastcall_header *)fargs);
} // }}}
static ssize_t data_binstring_t_free(data_t *data, fastcall_free *fargs){ // {{{
	data_binstring_t_destroy(data);
	return 0;
} // }}}
static ssize_t data_binstring_t_length(data_t *data, fastcall_length *fargs){ // {{{
	ssize_t                    ret;
	binstring_t               *fdata             = (binstring_t *)data->ptr;
	
	switch(fargs->format){
		case FORMAT(packed):;
		case FORMAT(binstring):;
			fastcall_length r_len = { { 3, ACTION_LENGTH }, 0, FORMAT(native) };
			if( (ret = data_query(fdata->data, &r_len)) < 0)
				return ret;
			
			fargs->length = sizeof(BINSTRING_SIZE) + r_len.length; // <size><data>
			break;
			
		default:
			ret = data_query(fdata->data, fargs);
			break;
		
	}
	return ret;
} // }}}
static ssize_t data_binstring_t_control(data_t *data, fastcall_control *fargs){ // {{{
	binstring_t           *fdata             = (binstring_t *)data->ptr;
	
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

data_proto_t binstring_t_proto = {
	.type            = TYPE_BINSTRINGT,
	.type_str        = "binstring_t",
	.api_type        = API_HANDLERS,
	.properties      = DATA_PROXY,
	.handler_default = (f_data_func)&data_binstring_t_handler,
	.handlers        = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_binstring_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_binstring_t_convert_from,
		[ACTION_LENGTH]       = (f_data_func)&data_binstring_t_length,
		[ACTION_FREE]         = (f_data_func)&data_binstring_t_free,
		[ACTION_CONTROL]      = (f_data_func)&data_binstring_t_control,
	}
};

