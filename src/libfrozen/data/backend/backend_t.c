#include <libfrozen.h>
#include <backend_t.h>
#include <uint/uint32_t.h>
#include <uint/off_t.h>
#include <uint/size_t.h>
#include <raw/raw_t.h>
#include <string/string_t.h>
#include <hash/hash_t.h>

static ssize_t   data_backend_t_read  (data_t *data, void *args){
	fastcall_read         *fargs             = (fastcall_read *)args;
	
	if(fargs->header->nargs < 5 || fargs->buffer == NULL)
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
}

static ssize_t   data_backend_t_write (data_t *data, void *args){
	fastcall_read         *fargs             = (fastcall_read *)args;
	
	if(fargs->header->nargs < 5 || fargs->buffer == NULL)
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
}

static ssize_t data_backend_t_convert(data_t *dst, void *args){ // {{{
	ssize_t                ret;
	data_t                *src;
	hash_t                *backend_config    = NULL;
	char                  *backend_name      = NULL;
	fastcall_convert      *fargs             = (fastcall_convert *)args;
	
	if(fargs->header->nargs < 3 || fargs->src == NULL)
		return -EINVAL;
	
	src = (data_t *)fargs->src;
	
	switch( src->type ){
		case TYPE_STRINGT: dst->data_ptr = backend_find( (char *)src->ptr );  break;
		case TYPE_HASHT:   dst->data_ptr = backend_new( (hash_t *)src->ptr ); break;
		default: break;
	};
	return -ENOSYS;
} // }}}

static ssize_t data_backend_t_len(data_t *data, void *args){
	fastcall_len          *fargs             = (fastcall_len *)args;
	
	if(fargs->header->nargs < 3)
		return -EINVAL;

	fargs->length = 0;
	return 0;
}

data_proto_t backend_t_proto = {
	.type                   = TYPE_BACKENDT,
	.type_str               = "backend_t",
	.api_type               = API_HANDLERS,
	.handlers = {
		[ACTION_READ]        = &data_backend_t_read,
		[ACTION_WRITE]       = &data_backend_t_write,
		[ACTION_CONVERT]     = &data_backend_t_convert,
		[ACTION_PHYSICALLEN] = &data_backend_t_len,
		[ACTION_LOGICALLEN]  = &data_backend_t_len
	}
};
