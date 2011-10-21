#include <libfrozen.h>
#include <dataptr_t.h>

static ssize_t data_dataptr_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = sizeof(data_t);
	return 0;
} // }}}
static ssize_t data_dataptr_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	if(fargs->src == NULL)
		return -EINVAL;
	
	dst->ptr = fargs->src;
	return 0;
} // }}}
static ssize_t data_dataptr_t_init(data_t *dst, fastcall_init *fargs){ // {{{
	dst->ptr = NULL;
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
	.handlers               = {
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_dataptr_t_len,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_dataptr_t_len,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_dataptr_t_convert_from,
		[ACTION_INIT]         = (f_data_func)&data_dataptr_t_init,
		[ACTION_FREE]         = (f_data_func)&data_dataptr_t_free,
	}
};
