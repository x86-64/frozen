#include <libfrozen.h>
#include <netstring_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>
#include <modifiers/slider/slider_t.h>

netstring_t *        netstring_t_alloc(data_t *data){ // {{{
	netstring_t                 *fdata;
	
	if( (fdata = malloc(sizeof(netstring_t))) == NULL )
		return NULL;
	
	fdata->data       = data;
	data_set_void(&fdata->freeit);
	return fdata;
} // }}}
void                 netstring_t_destroy(netstring_t *netstring){ // {{{
	data_free(&netstring->freeit);
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
			
			// <data>
			data_slider_t_freeze(&sl_dest);
				
				fastcall_convert_to r_net_convert = { { 5, ACTION_CONVERT_TO }, &sl_dest, FORMAT(native) };
				ret         = data_query(fdata->data, &r_net_convert);
				transfered += r_net_convert.transfered;
				
			data_slider_t_unfreeze(&sl_dest);
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
			data_t                 data;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if(fdata != NULL) // we already inited - pass
				break;

			hash_holder_consume(ret, data, config, HK(data));
			if(ret != 0)
				return -EINVAL;
			
			if( (fdata = dst->ptr = netstring_t_alloc(&data)) == NULL){
				data_free(&data);
				return -ENOMEM;
			}
			
			fdata->freeit = data;
			fdata->data   = &fdata->freeit;
			return 0;

		case FORMAT(netstring):
		case FORMAT(packed):
			// TODO unpack
			return -ENOSYS;

		default:
			break;
	}
	return data_netstring_t_handler(dst, (fastcall_header *)fargs);
} // }}}
static ssize_t data_netstring_t_free(data_t *data, fastcall_free *fargs){ // {{{
	netstring_t                  *fdata             = (netstring_t *)data->ptr;
	
	netstring_t_destroy(fdata);
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

data_proto_t netstring_t_proto = {
	.type            = TYPE_NETSTRINGT,
	.type_str        = "netstring_t",
	.api_type        = API_HANDLERS,
	.handler_default = (f_data_func)&data_netstring_t_handler,
	.handlers        = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_netstring_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_netstring_t_convert_from,
		[ACTION_LENGTH]       = (f_data_func)&data_netstring_t_length,
		[ACTION_FREE]         = (f_data_func)&data_netstring_t_free,
	}
};

