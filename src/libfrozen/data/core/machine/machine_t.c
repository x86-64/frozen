#include <libfrozen.h>
#include <machine_t.h>

#include <core/string/string_t.h>
#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <core/default/default_t.h>

static ssize_t data_machine_t_default(data_t *data, void *hargs){ // {{{
	return machine_fast_query((machine_t *)data->ptr, hargs);
} // }}}
static ssize_t data_machine_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	switch( fargs->src->type ){
		case TYPE_HASHT: dst->ptr = shop_new( (hash_t *)fargs->src->ptr ); goto check;
		default:
			break;
	}
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			dst->ptr = shop_new(config);
			goto check;
		
		case FORMAT(config):;
		case FORMAT(human):;      // TODO data_convert call with FORMAT(clean) :(
		default:;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read) != 0)
				return -EFAULT;
			
			buffer[r_read.buffer_size] = '\0';
			
			dst->ptr = machine_find(buffer);
			goto check;
	}

check:
	if(dst->ptr != NULL)
		return 0;
	
	return -EFAULT;
} // }}}
static ssize_t data_machine_t_len(data_t *data, fastcall_length *fargs){ // {{{
	fargs->length = 0;
	return 0;
} // }}}
static ssize_t data_machine_t_copy(data_t *src, fastcall_copy *fargs){ // {{{
	if(src->ptr == NULL || fargs->dest == NULL)
		return -EINVAL;
	
	fargs->dest->ptr = src->ptr;
	machine_acquire(src->ptr);
	return 0;
} // }}}
static ssize_t data_machine_t_free(data_t *data, fastcall_free *fargs){ // {{{
	shop_destroy((machine_t *)data->ptr);
	return 0;
} // }}}
static ssize_t data_machine_t_transfer(data_t *data, fastcall_transfer *fargs){ // {{{
	ssize_t                ret;
	
	ret = machine_fast_query((machine_t *)data->ptr, (fastcall_header *)fargs);
	if(errors_is_unix(ret, ENOSYS)){
		// no fastcall transfer support avaliable, use default_t handler
		
		fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, fargs->dest, FORMAT(clean) };
		if( (ret = data_protos[ TYPE_DEFAULTT ]->handlers[ ACTION_CONVERT_TO ](data, &r_convert)) != 0)
			return ret;
		
		if(fargs->header.nargs >= 4)
			fargs->transfered = r_convert.transfered;
	}
	return ret;
} // }}}
static ssize_t data_machine_t_query(data_t *data, fastcall_query *fargs){ // {{{
	return machine_query( (machine_t *)data->ptr, fargs->request );
} // }}}
static ssize_t data_machine_t_getdata(data_t *data, fastcall_getdata *fargs){ // {{{
	fargs->data = data;
	return 0;
} // }}}

// temprorary
static ssize_t data_machine_t_alloc(data_t *data, fastcall_alloc *fargs){ // {{{
	data->ptr = NULL;
	return 0;
} // }}}

data_proto_t machine_t_proto = {
	.type                   = TYPE_MACHINET,
	.type_str               = "machine_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_machine_t_default,
	.handlers = {
		[ACTION_LENGTH]       = (f_data_func)&data_machine_t_len,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_machine_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_machine_t_free,
		[ACTION_ALLOC]        = (f_data_func)&data_machine_t_alloc,
		[ACTION_TRANSFER]     = (f_data_func)&data_machine_t_transfer,
		[ACTION_QUERY]        = (f_data_func)&data_machine_t_query,
		[ACTION_COPY]         = (f_data_func)&data_machine_t_copy,
		[ACTION_GETDATA]      = (f_data_func)&data_machine_t_getdata,
	}
};
