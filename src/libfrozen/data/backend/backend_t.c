#include <libfrozen.h>
#include <dataproto.h>
#include <backend_t.h>

#include <string/string_t.h>
#include <hash/hash_t.h>
#include <format/format_t.h>

static ssize_t data_backend_t_io  (data_t *data, fastcall_io *fargs){ // {{{
	return backend_fast_query((backend_t *)data->ptr, fargs);
} // }}}
static ssize_t data_backend_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	switch( fargs->src->type ){
		case TYPE_HASHT: dst->ptr = backend_new( (hash_t *)fargs->src->ptr ); goto check;
		default:
			break;
	}
	switch(fargs->format){
		case FORMAT(hash):
			dst->ptr = backend_new( (hash_t *)fargs->src->ptr ); goto check;
			goto check;

		case FORMAT(human):;      // TODO data_convert call with FORMAT(clean) :(
		default:;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read) != 0)
				return -EFAULT;
			
			dst->ptr = backend_acquire(buffer);
			goto check;
	}

check:
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
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_backend_t_len,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_backend_t_len,
		[ACTION_READ]         = (f_data_func)&data_backend_t_io,
		[ACTION_WRITE]        = (f_data_func)&data_backend_t_io,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_backend_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_backend_t_free,
		[ACTION_ALLOC]        = (f_data_func)&data_backend_t_alloc,
	}
};
