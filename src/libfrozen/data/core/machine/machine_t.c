#include <libfrozen.h>
#include <machine_t.h>

#include <core/string/string_t.h>
#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>

static ssize_t data_machine_t_default(data_t *data, void *hargs){ // {{{
	return machine_fast_query((machine_t *)data->ptr, hargs);
} // }}}
static ssize_t data_machine_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	switch(fargs->format){
		case FORMAT(native):;
			if(fargs->src->type != TYPE_MACHINET)
				return -EFAULT;
			dst->ptr = fargs->src->ptr;
			machine_acquire(dst->ptr);
			return 0;

		case FORMAT(hash):;
			hash_t                *config;
			list                  *shops;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if( (ret = shop_new(config, &shops)) < 0)
				return ret;
			
			dst->ptr = list_pop(shops);
			
			shop_list_destroy(shops); // destroy rest, no splitted machines in machine_t for now
			goto check;
		
		case FORMAT(config):;
		case FORMAT(human):;      // TODO data_convert call with FORMAT(native) :(
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
static ssize_t data_machine_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr)
		shop_destroy((machine_t *)data->ptr);
	return 0;
} // }}}
static ssize_t data_machine_t_query(data_t *data, fastcall_query *fargs){ // {{{
	return machine_query( (machine_t *)data->ptr, fargs->request );
} // }}}

data_proto_t machine_t_proto = {
	.type                   = TYPE_MACHINET,
	.type_str               = "machine_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handler_default        = (f_data_func)&data_machine_t_default,
	.handlers = {
		[ACTION_LENGTH]       = (f_data_func)&data_machine_t_len,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_machine_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_machine_t_free,
		[ACTION_QUERY]        = (f_data_func)&data_machine_t_query,
		[ACTION_VIEW]         = (f_data_func)&data_default_view,
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
	}
};
