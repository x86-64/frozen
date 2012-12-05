#include <libfrozen.h>
#include <fastcall_t.h>

#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <modifiers/slider/slider_t.h>
//#include <storage/raw/raw_t.h>

static ssize_t api_pack_fastcall_callback(data_t *output, request_t *request){ // {{{
	data_t                 d_request         = DATA_PTR_HASHT(request);

	fastcall_convert_to r_convert_to = { { 4, ACTION_CONVERT_TO }, output, FORMAT(packed) };
	return data_query(&d_request, &r_convert_to);
} // }}}
static ssize_t api_unpack_fastcall_callback(fastcall_header *output, fastcall_header *input){ // {{{
	uintmax_t              header_size;
	
	header_size = MIN(input->nargs, output->nargs);
	
	if(header_size > FASTCALL_NARGS_LIMIT)      // too many arguments
		return -EFAULT;
	
	header_size *= sizeof(output->nargs);
	
	memcpy(output, input, header_size);
	return 0;
} // }}}
static ssize_t api_unpack_fastcall_output_callback(fastcall_header *output, fastcall_header *input){ // {{{
	if( api_transfer_fastcall(input, output) == -ENOSYS ){
		uintmax_t              header_size;
		
		header_size = MIN(input->nargs, output->nargs);
		
		if(header_size > FASTCALL_NARGS_LIMIT)      // too many arguments
			return -EFAULT;
		
		header_size *= sizeof(output->nargs);
		
		memcpy(output, input, header_size);
		return 0;
	}
	return 0;
} // }}}

ssize_t        api_unpack_fastcall(data_t *input, fastcall_t *output){ // {{{
	ssize_t                ret;
	data_t                 d_request         = DATA_PTR_HASHT(NULL);
	
	fastcall_convert_from r_convert_from = { { 4, ACTION_CONVERT_FROM }, input, FORMAT(packed) };
	if( (ret = data_query(&d_request, &r_convert_from)) < 0)
		return ret;
	
	ret = api_convert_request_to_fastcall(output->hargs, data_get_ptr(&d_request), (f_hash_to_fast)api_unpack_fastcall_callback);
	
	output->d_request = d_request;
	return ret;
} // }}}
ssize_t        api_unpack_fastcall_output(data_t *input, fastcall_t *output){ // {{{
	ssize_t                ret;
	data_t                 d_request         = DATA_PTR_HASHT(NULL);
	
	fastcall_convert_from r_convert_from = { { 4, ACTION_CONVERT_FROM }, input, FORMAT(packed) };
	if( (ret = data_query(&d_request, &r_convert_from)) < 0)
		return ret;
	
	ret = api_convert_request_to_fastcall(output->hargs, data_get_ptr(&d_request), (f_hash_to_fast)api_unpack_fastcall_output_callback);
	
	output->d_request = d_request;
	return ret;
} // }}}

static ssize_t data_fastcall_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	fastcall_t            *fdata             = (fastcall_t *)src->ptr;
	
	switch(fargs->format){
		case FORMAT(input):;
			return api_convert_fastcall_to_request(fargs->dest, fdata->hargs, (f_fast_to_hash)api_pack_fastcall_callback);
			
		case FORMAT(output):;
			return api_convert_fastcall_to_request(fargs->dest, fdata->hargs, (f_fast_to_hash)api_pack_fastcall_callback);
	}
	return -ENOSYS;
} // }}}
static ssize_t data_fastcall_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	fastcall_t            *fdata             = (fastcall_t *)dst->ptr;
	
	switch(fargs->format){
		case FORMAT(input):;
			return api_unpack_fastcall(fargs->src, fdata);
		
		case FORMAT(output):;
			return api_unpack_fastcall_output(fargs->src, fdata);
	}
	
	return 0;
} // }}}
static ssize_t data_fastcall_t_free(data_t *data, fastcall_free *fargs){ // {{{
	fastcall_t            *fdata             = (fastcall_t *)data->ptr;
	
	data_free(&fdata->d_request);
	data_set_void(data);
	return 0;
} // }}}

data_proto_t fastcall_t_proto = {
	.type                   = TYPE_FASTCALLT,
	.type_str               = "fastcall_t",
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
