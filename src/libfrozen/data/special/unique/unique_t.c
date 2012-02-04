#include <libfrozen.h>
#include <unique_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>

static ssize_t data_unique_t_handler (data_t *data, fastcall_header *hargs){ // {{{
	unique_t             *fdata             = (unique_t *)data->ptr;
	
	return data_query(&fdata->data, hargs);
} // }}}
static ssize_t data_unique_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	unique_t              *fdata;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if( (fdata = data->ptr = malloc(sizeof(unique_t))) == NULL)
				return -EINVAL;
			
			hash_holder_consume(ret, fdata->data, config, HK(data));
			return ret;

		default:
			return -ENOSYS;
	};
	return 0;
} // }}}
static ssize_t data_unique_t_free(data_t *data, fastcall_free *fargs){ // {{{
	unique_t              *fdata             = (unique_t *)data->ptr;
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&fdata->data, &r_free);
	free(fdata);
	return 0;
} // }}}
static ssize_t data_unique_t_push(data_t *data, fastcall_push *fargs){ // {{{
	ssize_t                ret;
	unique_t              *fdata             = (unique_t *)data->ptr;
	
	// compare list with single data - if same item exist in list r_compare.result would be 0
	fastcall_compare r_compare = { { 4, ACTION_COMPARE }, fargs->data };
	if( (ret = data_query(&fdata->data, &r_compare)) < 0)
		return ret;
	
	if(r_compare.result == 0){
		// consume data
		fastcall_free r_free = { { 2, ACTION_FREE } };
		if( (ret = data_query(fargs->data, &r_free)) < 0)
			return ret;
		
		return 0;
	}
	
	// insert data, coz it is unique
	return data_query(&fdata->data, fargs);
} // }}}

data_proto_t unique_t_proto = {
	.type                   = TYPE_UNIQUET,
	.type_str               = "unique_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_unique_t_handler,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_unique_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_unique_t_free,
		[ACTION_PUSH]         = (f_data_func)&data_unique_t_push,
	}
};
