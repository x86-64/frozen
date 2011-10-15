#include <libfrozen.h>
#include <backend_t.h>
#include <string/string_t.h>
#include <hash/hash_t.h>

static ssize_t data_backend_t_io  (data_t *data, fastcall_io *fargs){ // {{{
	ssize_t                ret;
	
	if( (ret = backend_fast_query((backend_t *)data->ptr, fargs)) < 0)
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
		case TYPE_STRINGT: dst->ptr = backend_acquire( (char *)fargs->src->ptr );  break;
		case TYPE_HASHT:   dst->ptr = backend_new( (hash_t *)fargs->src->ptr );    break;
		default:
			return -ENOSYS;
	};
	if(dst->ptr != NULL)
		return 0;
	
	return -EFAULT;
} // }}}
static ssize_t data_backend_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = 0;
	return 0;
} // }}}
static ssize_t data_backend_t_free(data_t *data, fastcall_free *fargs){ // {{{
	backend_destroy((backend_t *)data->ptr);
	return 0;
} // }}}

// temprorary
static ssize_t data_backend_t_alloc(data_t *data, fastcall_alloc *fargs){ // {{{
	data->ptr = NULL;
	return 0;
} // }}}

data_proto_t backend_t_proto = {
	.type                   = TYPE_BACKENDT,
	.type_str               = "backend_t",
	.api_type               = API_HANDLERS,
	.handlers = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_backend_t_len,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_backend_t_len,
		[ACTION_READ]        = (f_data_func)&data_backend_t_io,
		[ACTION_WRITE]       = (f_data_func)&data_backend_t_io,
		[ACTION_CONVERT]     = (f_data_func)&data_backend_t_convert,
		[ACTION_FREE]        = (f_data_func)&data_backend_t_free,
		[ACTION_ALLOC]       = (f_data_func)&data_backend_t_alloc,
	}
};
