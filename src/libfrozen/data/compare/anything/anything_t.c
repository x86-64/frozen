#include <libfrozen.h>
#include <anything_t.h>

static ssize_t data_anything_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	dst->type = TYPE_ANYTHINGT;
	dst->ptr  = NULL;
	return 0;
} // }}}
static ssize_t data_anything_t_noerr(data_t *data, fastcall_header *hargs){ // {{{
	return 0;
} // }}}
static ssize_t data_anything_t_compare(data_t *data, fastcall_compare *fargs){ // {{{
	fargs->result = 0; // equal
	return 0;
} // }}}

data_proto_t anything_t_proto = {
	.type          = TYPE_ANYTHINGT,
	.type_str      = "anything_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_anything_t_convert_from,
		[ACTION_CONVERT_TO]   = (f_data_func)&data_anything_t_noerr,
		[ACTION_FREE]         = (f_data_func)&data_anything_t_noerr,
		[ACTION_COMPARE]      = (f_data_func)&data_anything_t_compare,
	}
};

