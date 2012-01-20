#include <libfrozen.h>
#include <slice_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>

static ssize_t make_data_copy(data_t *src, data_t **dst){ // {{{
	ssize_t                ret;
	data_t                *hdst;
	
	if( (hdst = malloc(sizeof(data_t))) == NULL)
		return -ENOMEM;
	
	hdst->type = src->type;
	hdst->ptr  = NULL;
	
	fastcall_copy r_copy = { { 3, ACTION_COPY }, hdst };
	if( (ret = data_query(src, &r_copy)) != 0){
		free(hdst);
		return ret;
	}
	*dst = hdst;
	return 0;
} // }}}

static ssize_t data_slice_t_handler (data_t *data, fastcall_header *fargs){ // {{{
	ssize_t                   ret;
	slice_t                  *fdata             = (slice_t *)data->ptr;
	
	switch(fargs->action){
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io         *ioargs     = (fastcall_io *)fargs;
			
			if(ioargs->offset > fdata->size)
				return -EINVAL;
			
			ioargs->buffer_size  = MIN(ioargs->buffer_size, fdata->size - ioargs->offset);
			
			if(ioargs->buffer_size == 0)
				return -1;
			
			ioargs->offset      += fdata->off;
			return data_query(fdata->data, fargs);
		
		case ACTION_TRANSFER:
		case ACTION_CONVERT_TO:
			return data_protos[ TYPE_DEFAULTT ]->handlers[ fargs->action ](data, fargs);
		
		case ACTION_CONVERT_FROM:;
			fastcall_convert_from *cargs = (fastcall_convert_from *)fargs;
			
			switch(cargs->format){
				case FORMAT(hash):;
					hash_t                *config;
					data_t                *input;
					uintmax_t              offset     = 0;
					uintmax_t              size       = ~0;
					
					data_get(ret, TYPE_HASHT, config, cargs->src);
					if(ret != 0)
						return -EINVAL;
					
					input = hash_data_find(config, HK(input));
					hash_data_get(ret, TYPE_UINTT, offset, config, HK(offset));
					hash_data_get(ret, TYPE_UINTT, size,   config, HK(size));
					
					if(input == NULL)
						return -EINVAL;
					
					if( (ret = make_data_copy(input, &input)) != 0)
						return ret;
					
					if(fdata == NULL){
						if( (fdata = data->ptr = malloc(sizeof(slice_t))) == NULL)
							return -ENOMEM;
					}
					fdata->data  = input;
					fdata->off   = offset;
					fdata->size  = size;
					return 0;

				default:
					break;
			}
			return -ENOSYS;
		
		case ACTION_COPY:;
			slice_t               *ddata;
			data_t                *dhdata;
			fastcall_copy         *cpargs            = (fastcall_copy *)fargs;
			
			if(fdata == NULL)
				return -EINVAL;
			
			dhdata = fdata->data;
			
			if( (ddata  = malloc(sizeof(slice_t))) == NULL)
				return -ENOMEM;
			
			if( (ret = make_data_copy(dhdata, &dhdata)) != 0){
				free(ddata);
				return ret;
			}
			
			ddata->data = dhdata;
			ddata->off  = fdata->off;
			ddata->size = fdata->size;
			
			cpargs->dest->type = data->type;
			cpargs->dest->ptr  = ddata;
			return 0;

		/*
		case ACTION_GETDATAPTR:
			if( (ret = data_query(fdata->data, fargs)) != 0)
				return ret;
			
			fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
			if( (ret = data_query(fdata->data, &r_len)) != 0)
				return ret;
			
			if(fdata->off >= r_len.length)
				return -EINVAL;
			
			((fastcall_getdataptr *)fargs)->ptr += fdata->off;
			return 0;
		*/

		case ACTION_LOGICALLEN:
		case ACTION_PHYSICALLEN:;
			fastcall_len         *largs     = (fastcall_len *)fargs;
			
			if( (ret = data_query(fdata->data, fargs)) != 0)
				return ret;
			
			if( fdata->off > largs->length ){
				largs->length = 0;
				return 0;
			}
			
			largs->length = MIN( fdata->size, largs->length - fdata->off );
			return 0;
		default:
			break;
	};
	return data_query(fdata->data, fargs);
} // }}}

data_proto_t slice_t_proto = {
	.type                   = TYPE_SLICET,
	.type_str               = "slice_t",
	.api_type               = API_DEFAULT_HANDLER,
	.handler_default        = (f_data_func)&data_slice_t_handler,
};
