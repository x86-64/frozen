#include <libfrozen.h>
#include <random_t.h>

static ssize_t data_random_t_handler (data_t *data, fastcall_header *hargs){ // {{{
	switch(hargs->action){
		case ACTION_READ:;
			fastcall_io         *fargs       = (fastcall_io *)hargs;
			long int            *buffer      = (long int *)fargs->buffer;
			uintmax_t            buffer_size = fargs->buffer_size;
			
			while(buffer_size >= sizeof(long int)){
				*buffer++    = random();
				
				buffer_size -= sizeof(long int);
			}
			while(buffer_size > 0){
				*((char *)buffer) = (char)random();
				
				buffer            = (long int *)(((char *)buffer) + 1);
				buffer_size--;
			}
			return 0;
		
		case ACTION_RESIZE:
		case ACTION_FREE:
		case ACTION_CONVERT_FROM:
			data->ptr = NULL;
			return 0;
		
		case ACTION_LENGTH:;
			fastcall_length *r_len = (fastcall_length *)hargs;
			r_len->length = ~0;
			return 0;

		default:
			break;
	};
	return -ENOSYS;
} // }}}

data_proto_t random_t_proto = {
	.type                   = TYPE_RANDOMT,
	.type_str               = "random_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_random_t_handler,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_default_convert_to,
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
	}
};
