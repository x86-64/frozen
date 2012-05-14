#include <libfrozen.h>
#include <dataptr_t.h>

#include <enum/format/format_t.h>

static ssize_t data_dataptr_t_len(data_t *data, fastcall_length *fargs){ // {{{
	if(fargs->format == FORMAT(packed)){
		fargs->length = sizeof(data_t);
		return 0;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_dataptr_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	if(fargs->src == NULL)
		return -EINVAL;
	
	switch(fargs->format){
		case FORMAT(config):;
			dst->ptr = NULL;
			break;
		
		default:
			dst->ptr = fargs->src;
			break;
	}
	return 0;
} // }}}
static ssize_t data_dataptr_t_free(data_t *data, fastcall_free *fargs){ // {{{
	data->ptr = NULL;
	return 0;
} // }}}

data_proto_t dataptr_t_proto = {
	.type                   = TYPE_DATAPTRT,
	.type_str               = "dataptr_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handlers               = {
		[ACTION_LENGTH]       = (f_data_func)&data_dataptr_t_len,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_dataptr_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_dataptr_t_free,
	}
};
