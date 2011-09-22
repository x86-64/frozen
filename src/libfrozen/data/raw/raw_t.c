#include <libfrozen.h>
#include <default/default_t.h>
#include <raw_t.h>

static ssize_t data_raw_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = ((raw_t *)data->ptr)->size;
	return 0;
} // }}}
static ssize_t data_raw_io(data_t *data, fastcall_io *fargs){ // {{{
	data_t                 new_data;
	
	new_data.type = data->type;
	new_data.ptr  = ((raw_t *)data->ptr)->ptr;
	
	return default_t_proto.handlers[fargs->header.action](&new_data, fargs);
} // }}}
// TODO compare

data_proto_t raw_t_proto = {
	.type          = TYPE_RAWT,
	.type_str      = "raw_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_raw_len,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_raw_len,
		[ACTION_READ]        = (f_data_func)&data_raw_io,
		[ACTION_WRITE]       = (f_data_func)&data_raw_io
	}
};

