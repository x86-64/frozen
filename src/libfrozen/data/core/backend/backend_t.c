#include <libfrozen.h>
#include <backend_t.h>

#include <core/string/string_t.h>
#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <core/default/default_t.h>

static ssize_t data_backend_t_default(data_t *data, void *hargs){ // {{{
	return backend_fast_query((backend_t *)data->ptr, hargs);
} // }}}
static ssize_t data_backend_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	backend_t             *backend;
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	switch( fargs->src->type ){
		case TYPE_HASHT: dst->ptr = backend_new( (hash_t *)fargs->src->ptr ); goto check;
		default:
			break;
	}
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			dst->ptr = backend_new(config);
			goto check;
		
		case FORMAT(config):;
		case FORMAT(human):;      // TODO data_convert call with FORMAT(clean) :(
		default:;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read) != 0)
				return -EFAULT;
			
			backend = backend_find(buffer);
			backend_acquire(backend);
			
			dst->ptr = backend;
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
static ssize_t data_backend_t_transfer(data_t *data, fastcall_transfer *fargs){ // {{{
	ssize_t                ret;
	
	if( (ret = backend_fast_query((backend_t *)data->ptr, fargs)) == -ENOSYS){
		// no fastcall transfer support avaliable, use default_t handler
		
		fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, fargs->dest, FORMAT(clean) };
		if( (ret = data_protos[ TYPE_DEFAULTT ]->handlers[ ACTION_CONVERT_TO ](data, &r_convert)) != 0)
			return ret;
		
		if(fargs->header.nargs >= 4)
			fargs->transfered = r_convert.transfered;
	}
	return ret;
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
	.handler_default        = (f_data_func)&data_backend_t_default,
	.handlers = {
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_backend_t_len,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_backend_t_len,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_backend_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_backend_t_free,
		[ACTION_ALLOC]        = (f_data_func)&data_backend_t_alloc,
		[ACTION_TRANSFER]     = (f_data_func)&data_backend_t_transfer,
	}
};
