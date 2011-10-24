#include <libfrozen.h>
#include <dataproto.h>
#include <slice_t.h>

static ssize_t data_slice_t_handler (data_t *data, fastcall_header *fargs){ // {{{
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
