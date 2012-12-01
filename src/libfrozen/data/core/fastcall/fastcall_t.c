#include <libfrozen.h>
#include <fastcall_t.h>

#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <modifiers/slider/slider_t.h>
//#include <storage/raw/raw_t.h>

static ssize_t data_fastcall_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	fastcall_t            *fdata             = (fastcall_t *)src->ptr;
	
	switch(fargs->format){
		case FORMAT(input):;
			return api_pack_fastcall(fdata->hargs, fargs->dest);
			
		case FORMAT(output):;
			return api_pack_fastcall_output(fdata->hargs, fargs->dest);
	}
	return -ENOSYS;
} // }}}
static ssize_t data_fastcall_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	fastcall_t            *fdata             = (fastcall_t *)dst->ptr;
	
	switch(fargs->format){
		case FORMAT(input):;
			return api_unpack_fastcall(fargs->src, fdata->hargs);
		
		case FORMAT(output):;
			return api_unpack_fastcall_output(fargs->src, fdata->hargs);
	}
	
	return 0;
} // }}}
static ssize_t data_fastcall_t_free(data_t *data, fastcall_free *fargs){ // {{{
	return 0;
} // }}}

data_proto_t fastcall_t_proto = {
	.type                   = TYPE_FASTCALLT,
	//.type_str               = "fastcall_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handlers = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_fastcall_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_fastcall_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_fastcall_t_free,
		[ACTION_VIEW]         = (f_data_func)&data_default_view,
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
	}
};
