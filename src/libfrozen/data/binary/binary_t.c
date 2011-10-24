#include <libfrozen.h>
#include <dataproto.h>
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
static ssize_t data_binary_t_getdataptr(data_t *data, fastcall_getdataptr *fargs){ // {{{
	fargs->ptr = (void *) ( ((data_binary_t *)data->ptr) + 1 );
	return 0;
} // }}}
static ssize_t data_binary_t_transfer(data_t *src, fastcall_transfer *fargs){ // {{{
	ssize_t                ret;

	if(fargs->dest == NULL)
		return -EINVAL;
	
	fastcall_write r_write = { { 5, ACTION_WRITE }, 0, (((data_binary_t *)src->ptr) + 1), ((data_binary_t *)src->ptr)->size };
	if( (ret = data_query(fargs->dest, &r_write)) < -1)
		return ret;
	
	return 0;
} // }}}

data_proto_t binary_t_proto = {
	.type            = TYPE_BINARYT,
	.type_str        = "binary_t",
	.api_type        = API_HANDLERS,
	.handlers        = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_binary_t_physlen,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_binary_t_loglen,
		[ACTION_GETDATAPTR]  = (f_data_func)&data_binary_t_getdataptr,
		[ACTION_TRANSFER]    = (f_data_func)&data_binary_t_transfer,
	}
};

