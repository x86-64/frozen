#include <libfrozen.h>
#include <immortal_t.h>

static ssize_t data_immortal_t_handler(data_t *data, fastcall_header *hargs){ // {{{
	immortal_t            *fdata             = (immortal_t *)data->ptr;
	
	return data_query(fdata->data, hargs);
} // }}}
static ssize_t data_immortal_t_free(data_t *data, fastcall_free *hargs){ // {{{
	return 0;
} // }}}

data_proto_t immortal_t_proto = {
	.type_str               = "immortal_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_immortal_t_handler,
	.handlers               = {
		[ACTION_FREE]         = (f_data_func)&data_immortal_t_free,
	}
};
