#include <libfrozen.h>
#include <backend_t.h>
#include <string/string_t.h>
#include <hash/hash_t.h>

static ssize_t data_backend_t_read  (data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->buffer == NULL)
		return -EINVAL;
	
	if( (ret = backend_stdcall_read(
		(backend_t *)data->ptr,
		fargs->offset,
		fargs->buffer,
		fargs->buffer_size
	)) < 0)
		return ret;
	if(ret == 0)
		return -1; // EOF
	
	fargs->buffer_size = ret;
	return 0;
} // }}}
static ssize_t data_backend_t_write (data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->buffer == NULL)
		return -EINVAL;
	
	if( (ret = backend_stdcall_write(
		(backend_t *)data->ptr,
		fargs->offset,
		fargs->buffer,
		fargs->buffer_size
	)) < 0)
		return ret;
	if(ret == 0)
		return -1; // EOF
	
	fargs->buffer_size = ret;
	return 0;
} // }}}
static ssize_t data_backend_t_convert(data_t *dst, fastcall_convert *fargs){ // {{{
	if(fargs->src == NULL)
		return -EINVAL;
	
	switch( fargs->src->type ){
		case TYPE_STRINGT: dst->ptr = backend_find( (char *)fargs->src->ptr );  break;
		case TYPE_HASHT:   dst->ptr = backend_new( (hash_t *)fargs->src->ptr ); break;
		default: break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_backend_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = 0;
	return 0;
} // }}}

data_proto_t backend_t_proto = {
	.type                   = TYPE_BACKENDT,
	.type_str               = "backend_t",
	.api_type               = API_HANDLERS,
	.handlers = {
		[ACTION_READ]        = (f_data_func)&data_backend_t_read,
		[ACTION_WRITE]       = (f_data_func)&data_backend_t_write,
		[ACTION_CONVERT]     = (f_data_func)&data_backend_t_convert,
		[ACTION_PHYSICALLEN] = (f_data_func)&data_backend_t_len,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_backend_t_len
	}
};
