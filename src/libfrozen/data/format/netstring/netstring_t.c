#include <libfrozen.h>
#include <netstring_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>
#include <modifiers/slider/slider_t.h>
#include <modifiers/slice/slice_t.h>

ssize_t data_netstring_t(data_t *data, data_t storage){ // {{{
	netstring_t           *fdata;
	
	if( (fdata = malloc(sizeof(netstring_t))) == NULL )
		return -ENOMEM;
	
	fdata->storage = storage;
	fdata->data    = &fdata->storage;
	
	data->type = TYPE_NETSTRINGT;
	data->ptr  = fdata;
	return 0;
} // }}}
void    data_netstring_t_destroy(data_t *data){ // {{{
	netstring_t           *fdata             = (netstring_t *)data->ptr;
	
	if(fdata){
		data_free(&fdata->storage);
		free(fdata);
	}
	data_set_void(data);
} // }}}

static ssize_t data_netstring_t_handler (data_t *data, fastcall_header *fargs){ // {{{
	netstring_t               *fdata             = (netstring_t *)data->ptr;

	return data_query(fdata->data, fargs);
} // }}}
static ssize_t data_netstring_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	char                   buffer[100];
	intmax_t               buffer_size;
	uintmax_t              transfered        = 0;
	netstring_t           *fdata             = (netstring_t *)src->ptr;
	data_t                 sl_dest           = DATA_SLIDERT(fargs->dest, 0);
	
	switch(fargs->format){
		case FORMAT(packed):;
		case FORMAT(netstring):;
			fastcall_length r_len = { { 3, ACTION_LENGTH }, 0, FORMAT(native) };
			if( (ret = data_query(fdata->data, &r_len)) < 0)
				return ret;
			
			buffer_size = snprintf(buffer, sizeof(buffer), "%" SCNuMAX ":", r_len.length);
			if( buffer_size <= 0 || buffer_size >= sizeof(buffer) )
				return -EINVAL;
			
			// <size>:
			fastcall_write r_net_write1 = { { 5, ACTION_WRITE }, 0, buffer, buffer_size };
			ret         = data_query(&sl_dest, &r_net_write1);
			transfered += r_net_write1.buffer_size;	
			if(ret < 0) break;
			data_slider_t_set_offset(&sl_dest, r_net_write1.buffer_size, SEEK_CUR);
			
			// <data>
			
			fastcall_convert_to r_net_convert = { { 5, ACTION_CONVERT_TO }, &sl_dest, FORMAT(native) };
			ret         = data_query(fdata->data, &r_net_convert);
			transfered += r_net_convert.transfered;
			
			data_slider_t_set_offset(&sl_dest, r_net_convert.transfered, SEEK_CUR);
			if(ret < 0) break;
			
			// ,
			fastcall_write r_net_write2 = { { 5, ACTION_WRITE }, 0, ",", 1 };
			ret         = data_query(&sl_dest, &r_net_write2);
			transfered += r_net_write2.buffer_size;
			break;
			
		default:
			return data_query(fdata->data, fargs);
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_netstring_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	netstring_t           *fdata             = (netstring_t *)dst->ptr;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			data_t                 storage;
			
			if(fdata != NULL) // we already inited - pass
				break;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			hash_holder_consume(ret, storage, config, HK(data));
			if(ret != 0)
				return -EINVAL;
			
			if( (ret = data_netstring_t(dst, storage)) < 0){
				data_free(&storage);
				return -ENOMEM;
			}
			return 0;
			
		case FORMAT(native):;
			data_t                     fargs_src_storage;
			netstring_t               *fargs_src        = (netstring_t *)fargs->src->ptr;
			
			if(fdata != NULL) // we already inited - pass
				break;
			
			if(dst->type != fargs->src->type)
				return -EINVAL;
			
			holder_copy(ret, &fargs_src_storage, &fargs_src->storage);
			if(ret < 0)
				return ret;
			
			if( (ret = data_netstring_t(dst, fargs_src_storage)) < 0){
				data_free(&fargs_src_storage);
				return ret;
			}
			return 0;
			
		case FORMAT(netstring):
		case FORMAT(packed):;
			uintmax_t              size;
			data_t                 d_size           = DATA_UINTT(0);
			
			fastcall_convert_from r_convert_size = { { 5, ACTION_CONVERT_FROM }, fargs->src, FORMAT(human) };
			if( (ret = data_query(&d_size, &r_convert_size)) < 0)
				return ret;

			size = DEREF_TYPE_UINTT(&d_size);
			
			data_t d_slice = DATA_SLICET(fargs->src, r_convert_size.transfered + 1, size); // 123:
			
			fargs->src = &d_slice;
			ret = data_query(fdata->data, fargs);
			
			fargs->transfered += r_convert_size.transfered + 2; // 123:data,
			return ret;

		default:
			break;
	}
	return data_netstring_t_handler(dst, (fastcall_header *)fargs);
} // }}}
static ssize_t data_netstring_t_free(data_t *data, fastcall_free *fargs){ // {{{
	data_netstring_t_destroy(data);
	return 0;
} // }}}
static ssize_t data_netstring_t_length(data_t *data, fastcall_length *fargs){ // {{{
	ssize_t                    ret;
	char                       buffer[30];
	intmax_t                   buffer_size;
	netstring_t               *fdata             = (netstring_t *)data->ptr;
	
	switch(fargs->format){
		case FORMAT(packed):;
		case FORMAT(netstring):;
			fastcall_length r_len = { { 3, ACTION_LENGTH }, 0, FORMAT(native) };
			if( (ret = data_query(fdata->data, &r_len)) < 0)
				return ret;
			
			buffer_size = snprintf(buffer, sizeof(buffer), "%" SCNuMAX ":", r_len.length);
			if( buffer_size <= 0 || buffer_size >= sizeof(buffer) )
				return -EINVAL;
			
			fargs->length = buffer_size + r_len.length + 1; // <size>:<data>,
			break;
			
		default:
			ret = data_query(fdata->data, fargs);
			break;
		
	}
	return ret;
} // }}}
static ssize_t data_netstring_t_control(data_t *data, fastcall_control *fargs){ // {{{
	netstring_t           *fdata             = (netstring_t *)data->ptr;
	
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

data_proto_t netstring_t_proto = {
	.type            = TYPE_NETSTRINGT,
	.type_str        = "netstring_t",
	.api_type        = API_HANDLERS,
	.properties      = DATA_PROXY,
	.handler_default = (f_data_func)&data_netstring_t_handler,
	.handlers        = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_netstring_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_netstring_t_convert_from,
		[ACTION_LENGTH]       = (f_data_func)&data_netstring_t_length,
		[ACTION_FREE]         = (f_data_func)&data_netstring_t_free,
		[ACTION_CONTROL]      = (f_data_func)&data_netstring_t_control,
	}
};

