#include <libfrozen.h>
#include <slice_t.h>

static ssize_t data_slice_t_read  (data_t *data, void *fargs){ // {{{
	slice_t                  *fdata             = (slice_t *)data->ptr;
	// TODO off + size limit
	return data_query(fdata->data, fargs);
} // }}}
static ssize_t data_slice_t_write (data_t *data, void *fargs){ // {{{
	slice_t                  *fdata             = (slice_t *)data->ptr;
	// TODO off + size limit
	return data_query(fdata->data, fargs);
} // }}}

data_proto_t slice_t_proto = {
	.type                   = TYPE_SLICET,
	.type_str               = "slice_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_READ]   = (f_data_func)&data_slice_t_read,
		[ACTION_WRITE]  = (f_data_func)&data_slice_t_write
	}
};
