#include <libfrozen.h>
#include <slider_t.h>

uintmax_t data_slider_t_get_offset(data_t *data){ // {{{
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	return fdata->off;
} // }}}
void      data_slider_t_freeze(data_t *data){ // {{{
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	fdata->frozen_off = fdata->off;
} // }}}
void      data_slider_t_unfreeze(data_t *data){ // {{{
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	fdata->frozen_off = ~0;
} // }}}

static ssize_t data_slider_t_handler (data_t *data, fastcall_header *fargs){ // {{{
	size_t                     ret;
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	switch(fargs->action){
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io         *ioargs     = (fastcall_io *)fargs;
			
			if(fdata->frozen_off != ~0)
				ioargs->offset      += fdata->frozen_off;
			else
				ioargs->offset      += fdata->off;

			if( (ret = data_query(fdata->data, fargs)) >= 0)
				fdata->off += ioargs->buffer_size;

			return ret;
		
		case ACTION_TRANSFER:
		case ACTION_CONVERT_TO:
			return data_protos[ TYPE_DEFAULTT ]->handlers[ fargs->action ](data, fargs);

		case ACTION_CONVERT_FROM:
			return -ENOSYS;

		default:
			break;
	};
	return data_query(fdata->data, fargs);
} // }}}

data_proto_t slider_t_proto = {
	.type                   = TYPE_SLIDERT,
	.type_str               = "slider_t",
	.api_type               = API_DEFAULT_HANDLER,
	.handler_default        = (f_data_func)&data_slider_t_handler,
};
