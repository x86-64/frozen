#include <libfrozen.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>

#include <length_t.h>

static ssize_t data_length_t_copy(data_t *src, fastcall_copy *fargs){ // {{{
	ssize_t                ret;
	length_t              *fdata             = (length_t *)src->ptr;
	length_t              *new_fdata         = NULL;
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	if(fdata){
		if( (new_fdata = malloc(sizeof(length_t))) == NULL)
			return -EINVAL;
		
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &new_fdata->data };
		if( (ret = data_query(&fdata->data, &r_copy)) < 0){
			free(new_fdata);
			return ret;
		}
	}
	fargs->dest->type = src->type;
	fargs->dest->ptr  = new_fdata;
	return 0;
} // }}}
static ssize_t data_length_t_free(data_t *data, fastcall_free *hargs){ // {{{
	length_t              *fdata             = (length_t *)data->ptr;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&fdata->data, &r_free);
	
	free(fdata);
	data->ptr = NULL;
	return 0;
} // }}}
static ssize_t data_length_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	length_t              *fdata;
	
	if(dst->ptr != NULL)
		return -EINVAL;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return ret;
			
			if( (fdata = malloc(sizeof(length_t))) == NULL)
				return -ENOMEM;
			
			hash_holder_consume(ret, fdata->data, config, HK(data));
			if(ret != 0){
				free(fdata);
				return -EINVAL;
			}
			
			fdata->format = FORMAT(native);
			hash_data_get(ret, TYPE_FORMATT, fdata->format, config, HK(format));
			
			dst->ptr = fdata;
			return 0;
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
/*static ssize_t data_length_t_getdata(data_t *data, fastcall_getdata *fargs){ // {{{
	length_t              *fdata             = (length_t *)data->ptr;
	
	fargs->data = &fdata->data;
	return 0;
} // }}}*/
static ssize_t       data_length_t_getdataptr(data_t *data, fastcall_getdataptr *fargs){ // {{{
	return -EINVAL;
} // }}}
static ssize_t data_length_t_handler(data_t *data, fastcall_header *hargs){ // {{{
	ssize_t                ret;
	length_t              *fdata             = (length_t *)data->ptr;
	
	fastcall_length r_len = { { 4, ACTION_LENGTH }, 0, fdata->format };
	if( (ret = data_query(&fdata->data, &r_len)) < 0)
		return ret;
	
	data_t                 d_length          = DATA_PTR_UINTT(&r_len.length);
	return data_query(&d_length, hargs);
} // }}}

data_proto_t length_t_proto = {
	.type_str               = "length_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_COPY]         = (f_data_func)&data_length_t_copy,
		[ACTION_FREE]         = (f_data_func)&data_length_t_free,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_length_t_convert_from,
		//[ACTION_GETDATA]      = (f_data_func)&data_length_t_getdata,
		[ACTION_GETDATAPTR]   = (f_data_func)&data_length_t_getdataptr,
		[ACTION_READ]         = (f_data_func)&data_length_t_handler,
		[ACTION_CONVERT_TO]   = (f_data_func)&data_length_t_handler,
		[ACTION_LENGTH]       = (f_data_func)&data_length_t_handler,
	}
};
