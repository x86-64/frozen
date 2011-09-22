#include <libfrozen.h>
#include <io_t.h>

static ssize_t data_io_t_read  (data_t *data, void *fargs){ // {{{
	io_t                  *fdata             = (io_t *)data->ptr;
	return fdata->read(&fdata->ud, fargs);
} // }}}
static ssize_t data_io_t_write (data_t *data, void *fargs){ // {{{
	io_t                  *fdata             = (io_t *)data->ptr;
	return fdata->write(&fdata->ud, fargs);
} // }}}

data_proto_t io_t_proto = {
	.type                   = TYPE_IOT,
	.type_str               = "io_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_READ]   = (f_data_func)&data_io_t_read,
		[ACTION_WRITE]  = (f_data_func)&data_io_t_write
	}
};
