#include <libfrozen.h>
#include <default/default_t.h>
#include <binary_t.h>

typedef struct data_binary_t {
	uintmax_t              size;
} data_binary_t;

static ssize_t data_binary_t_physlen(data_t *data, fastcall_physicallen *fargs){ // {{{
	data_binary_t         *fdata             = (data_binary_t *)data->ptr;

	fargs->length = fdata->size + sizeof(data_binary_t);
	return 0;
} // }}}
static ssize_t data_binary_t_loglen(data_t *data, fastcall_logicallen *fargs){ // {{{
	data_binary_t         *fdata             = (data_binary_t *)data->ptr;
	
	fargs->length = fdata->size;
	return 0;
} // }}}
static ssize_t data_binary_t_io(data_t *data, fastcall_io *fargs){ // {{{
	data_t                 new_data;
	
	new_data.type = data->type;
	new_data.ptr  = (void *) ( ((data_binary_t *)data->ptr) + 1 );
	return default_t_proto.handlers[fargs->header.action](&new_data, fargs);
} // }}}

data_proto_t binary_t_proto = {
	.type            = TYPE_BINARYT,
	.type_str        = "binary_t",
	.api_type        = API_HANDLERS,
	.handlers        = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_binary_t_physlen,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_binary_t_loglen,
		[ACTION_READ]        = (f_data_func)&data_binary_t_io,
		[ACTION_WRITE]       = (f_data_func)&data_binary_t_io
	}
};

