#include <libfrozen.h>
#include <void_t.h>

static ssize_t data_void_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	dst->type = TYPE_VOIDT;
	dst->ptr  = NULL;
	return 0;
} // }}}
static ssize_t data_void_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	return 0;
} // }}}
static ssize_t data_void_t_push(data_t *data, fastcall_push *fargs){ // {{{
	return 0;
} // }}}
static ssize_t data_void_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	fastcall_push r_push = { { 3, ACTION_PUSH }, NULL };
	data_query(fargs->dest, &r_push);
	return 0;
} // }}}

data_proto_t void_t_proto = {
	.type          = TYPE_VOIDT,
	.type_str      = "void_t",
	.api_type      = API_HANDLERS,
	.properties    = DATA_ENDPOINT,
	.handlers      = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_void_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_void_t_convert_from,
		[ACTION_PUSH]         = (f_data_func)&data_void_t_push,
		[ACTION_ENUM]         = (f_data_func)&data_void_t_enum,
	}
};

