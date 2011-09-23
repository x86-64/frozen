#include <libfrozen.h>
#include <string_t.h>

static ssize_t data_string_t_physlen(data_t *data, fastcall_physicallen *fargs){ // {{{
	fargs->length = strlen((char *)data->ptr) + 1;
	return 0;
} // }}}
static ssize_t data_string_t_loglen(data_t *data, fastcall_logicallen *fargs){ // {{{
	fargs->length = strlen((char *)data->ptr);
	return 0;
} // }}}

data_proto_t string_t_proto = {
	.type          = TYPE_STRINGT,
	.type_str      = "string_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_string_t_physlen,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_string_t_loglen
	}
};
