#include <libfrozen.h>
#include <dataproto.h>
#include <slice_t.h>

static ssize_t data_slice_t_handler (data_t *data, fastcall_header *fargs){ // {{{
	ssize_t                   ret;
	slice_t                  *fdata             = (slice_t *)data->ptr;
	
	switch(fargs->action){
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io         *ioargs     = (fastcall_io *)fargs;
			
			ioargs->offset      += fdata->off;
			ioargs->buffer_size  = MIN(ioargs->buffer_size, fdata->size);
			return data_query(fdata->data, fargs);
		
		case ACTION_TRANSFER:
			return data_protos[ TYPE_DEFAULTT ]->handlers[ ACTION_TRANSFER ](data, fargs);
		
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
