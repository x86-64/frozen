#include <libfrozen.h>
#include <slice_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>

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
					data_t                 input;
					hash_t                *config;

					data_get(ret, TYPE_HASHT, config, cargs->src);
					if(ret != 0)
						return -EINVAL;
					
					if(fdata == NULL){
						if( (fdata = data->ptr = malloc(sizeof(slice_t))) == NULL)
							return -ENOMEM;
						
						fdata->off  = 0;
						fdata->size = ~0;
					}
					
					hash_data_get(ret, TYPE_UINTT, fdata->off,    config, HK(offset));
					hash_data_get(ret, TYPE_UINTT, fdata->size,   config, HK(size));
					hash_holder_consume(ret, input, config, HK(input));
					if(ret != 0)
						return -EINVAL;
					
					if( (fdata->data = malloc(sizeof(data_t))) == NULL)
						return -ENOMEM;

					*fdata->data = input;
					return 0;

				default:
					break;
			}
			return -ENOSYS;
		
/*		case ACTION_COPY:;
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
			return 0; */

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
