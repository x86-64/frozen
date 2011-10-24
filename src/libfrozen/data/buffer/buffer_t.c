#include <libfrozen.h>
#include <dataproto.h>
#include <buffer_t.h>

static ssize_t data_buffer_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = buffer_get_size( (buffer_t *)data->ptr );
	return 0;
} // }}}
static ssize_t data_buffer_t_read(data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->buffer == NULL)
		return -EINVAL;

	if( (ret = buffer_read( (buffer_t *)data->ptr, fargs->offset, fargs->buffer, fargs->buffer_size)) == -1){
		fargs->buffer_size = 0;
		return -1; // EOF
	}
	fargs->buffer_size = ret;
	return 0;
} // }}}
static ssize_t data_buffer_t_write (data_t *data, fastcall_write *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->buffer == NULL)
		return -EINVAL;
	
	if( (ret = buffer_write( (buffer_t *)data->ptr, fargs->offset, fargs->buffer, fargs->buffer_size)) == -1){
		fargs->buffer_size = 0;
		return -1; // EOF
	}
	fargs->buffer_size = ret;
	return 0;
} // }}}

data_proto_t buffer_t_proto = {
	.type                   = TYPE_BUFFERT,
	.type_str               = "buffer_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_buffer_t_len,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_buffer_t_len,
		[ACTION_READ]        = (f_data_func)&data_buffer_t_read,
		[ACTION_WRITE]       = (f_data_func)&data_buffer_t_write,
	}
};
/* vim: set filetype=c: */
