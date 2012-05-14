#include <libfrozen.h>
#include <io_t.h>

static ssize_t data_io_t_handler (data_t *data, fastcall_header *hargs){ // {{{
	io_t                  *fdata             = (io_t *)data->ptr;
	
	switch(hargs->action){
		default:
			return fdata->handler(data, fdata->ud, hargs);
	};
	return -ENOSYS;
} // }}}

data_proto_t io_t_proto = {
	.type                   = TYPE_IOT,
	.type_str               = "io_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handler_default        = (f_data_func)&data_io_t_handler,
	.handlers               = {
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
	}
};
