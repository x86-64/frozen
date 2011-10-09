#include <libfrozen.h>
#include <void_t.h>

static ssize_t data_void_t_convert(data_t *dst, fastcall_convert *fargs){ // {{{
	dst->type = TYPE_VOIDT;
	dst->ptr  = NULL;
	return 0;
} // }}}

data_proto_t void_t_proto = {
	.type          = TYPE_VOIDT,
	.type_str      = "void_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_CONVERT] = (f_data_func)&data_void_t_convert,
	}
};

