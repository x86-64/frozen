#include <libfrozen.h>
#include <raw_t.h>

static ssize_t data_raw_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = ((raw_t *)data->ptr)->size;
	return 0;
} // }}}
static ssize_t data_raw_getdataptr(data_t *data, fastcall_getdataptr *fargs){ // {{{
	fargs->ptr = ((raw_t *)data->ptr)->ptr;
	return 0;
} // }}}
static ssize_t data_raw_transfer(data_t *src, fastcall_transfer *fargs){ // {{{
	ssize_t                ret;

	if(fargs->dest == NULL)
		return -EINVAL;
	
	fastcall_write r_write = { { 5, ACTION_WRITE }, 0, ((raw_t *)src->ptr)->ptr, ((raw_t *)src->ptr)->size };
	if( (ret = data_query(fargs->dest, &r_write)) < -1)
		return ret;
	
	return 0;
} // }}}
// TODO compare

data_proto_t raw_t_proto = {
	.type          = TYPE_RAWT,
	.type_str      = "raw_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_raw_len,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_raw_len,
		[ACTION_GETDATAPTR]  = (f_data_func)&data_raw_getdataptr,
		[ACTION_TRANSFER]    = (f_data_func)&data_raw_transfer,
	}
};

