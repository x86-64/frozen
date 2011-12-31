#include <libfrozen.h>
#include <counter_t.h>

static ssize_t data_counter_t_handler (data_t *data, fastcall_header *hargs){ // {{{
	counter_t             *fdata             = (counter_t *)data->ptr;
	
	switch(hargs->action){
		case ACTION_READ:;
			fastcall_io         *fargs       = (fastcall_io *)hargs;
			
			fargs->buffer_size = MIN(fargs->buffer_size, sizeof(fdata->counter));
			memcpy(fargs->buffer, &fdata->counter, fargs->buffer_size);
			
			fdata->counter++;
			return 0;
		
		case ACTION_TRANSFER:
		case ACTION_CONVERT_TO:
			return data_protos[ TYPE_DEFAULTT ]->handlers[ hargs->action ](data, hargs);
			
		case ACTION_ALLOC:
			if( (fdata = data->ptr = malloc(sizeof(counter_t))) == NULL)
				return -ENOMEM;
			
			fdata->counter = 0;
			return 0;
		
		case ACTION_FREE:
			if(fdata)
				free(fdata);
			return 0;
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}


data_proto_t counter_t_proto = {
	.type                   = TYPE_COUNTERT,
	.type_str               = "counter_t",
	.api_type               = API_DEFAULT_HANDLER,
	.handler_default        = (f_data_func)&data_counter_t_handler,
};
