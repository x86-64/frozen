#include <libfrozen.h>
#include <binstring_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>
#include <modifiers/slider/slider_t.h>
#include <modifiers/slice/slice_t.h>

#define BINSTRING_SIZE uint32_t

binstring_t *        binstring_t_alloc(data_t *data){ // {{{
	binstring_t                 *fdata;
	
	if( (fdata = malloc(sizeof(binstring_t))) == NULL )
		return NULL;
	
	fdata->data       = data;
	data_set_void(&fdata->freeit);
	return fdata;
} // }}}
void                 binstring_t_destroy(binstring_t *binstring){ // {{{
	data_free(&binstring->freeit);
	free(binstring);
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
			data_t                 data;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if(fdata != NULL)          // we already inited - pass request
				break;
			
			hash_holder_consume(ret, data, config, HK(data));
			if(ret != 0)
				return -EINVAL;
			
			if( (fdata = dst->ptr = binstring_t_alloc(&data)) == NULL){
				data_free(&data);
				return -ENOMEM;
			}
			
			fdata->freeit = data;
			fdata->data   = &fdata->freeit;
			return 0;

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
		
		default:
			break;
	}
	
	return data_binstring_t_handler(dst, (fastcall_header *)fargs);
} // }}}
static ssize_t data_binstring_t_free(data_t *data, fastcall_free *fargs){ // {{{
	binstring_t                  *fdata             = (binstring_t *)data->ptr;
	
	binstring_t_destroy(fdata);
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

data_proto_t binstring_t_proto = {
	.type            = TYPE_BINSTRINGT,
	.type_str        = "binstring_t",
	.api_type        = API_HANDLERS,
	.handler_default = (f_data_func)&data_binstring_t_handler,
	.handlers        = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_binstring_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_binstring_t_convert_from,
		[ACTION_LENGTH]       = (f_data_func)&data_binstring_t_length,
		[ACTION_FREE]         = (f_data_func)&data_binstring_t_free,
	}
};

