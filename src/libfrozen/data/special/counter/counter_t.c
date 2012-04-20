#include <libfrozen.h>
#include <counter_t.h>

#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>

counter_t *    counter_t_alloc(data_t *data){ // {{{
	counter_t                 *fdata;

	if( (fdata = malloc(sizeof(counter_t))) == NULL )
		return NULL;
	
	fdata->counter = *data;
	return fdata;
} // }}}
void           counter_t_destroy(counter_t *counter){ // {{{
	data_free(&counter->counter);
} // }}}

static ssize_t data_counter_t_incandpass(data_t *data, fastcall_header *hargs){ // {{{
	ssize_t                ret;
	counter_t             *fdata             = (counter_t *)data->ptr;
	
	fastcall_increment r_inc = { { 2, ACTION_INCREMENT } };
	if( (ret = data_query(&fdata->counter, &r_inc)) < 0)
		return ret;
	
	return data_query(&fdata->counter, hargs);
} // }}}
static ssize_t data_counter_t_convert_to(data_t *data, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	counter_t             *fdata             = (counter_t *)data->ptr;
	
	if(fargs->format != FORMAT(packed)){ // binary means packed, so no increments
		fastcall_increment r_inc = { { 2, ACTION_INCREMENT } };
		if( (ret = data_query(&fdata->counter, &r_inc)) < 0)
			return ret;
	}

	return data_query(&fdata->counter, fargs);
} // }}}
static ssize_t data_counter_t_pass(data_t *data, fastcall_header *hargs){ // {{{
	counter_t             *fdata             = (counter_t *)data->ptr;
	
	return data_query(&fdata->counter, hargs);
} // }}}
static ssize_t data_counter_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(dst->ptr != NULL)
		return data_counter_t_pass(dst, (fastcall_header *)fargs);  // already inited - pass to underlying data
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			data_t                 data;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			hash_holder_consume(ret, data, config, HK(data));
			if(ret != 0)
				return -EINVAL;
			
			return ( (dst->ptr = counter_t_alloc(&data)) == NULL ) ? -EFAULT : 0;

		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_counter_t_free(data_t *data, fastcall_free *fargs){ // {{{
	counter_t_destroy(data->ptr);
	return 0;
} // }}}

data_proto_t counter_t_proto = {
	.type                   = TYPE_COUNTERT,
	.type_str               = "counter_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_counter_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_counter_t_free,
		[ACTION_CONVERT_TO]   = (f_data_func)&data_counter_t_convert_to,
		[ACTION_READ]         = (f_data_func)&data_counter_t_incandpass,
		[ACTION_INCREMENT]    = (f_data_func)&data_counter_t_pass,
		[ACTION_DECREMENT]    = (f_data_func)&data_counter_t_pass,
		[ACTION_LENGTH]       = (f_data_func)&data_counter_t_pass,
		[ACTION_ADD]          = (f_data_func)&data_counter_t_pass,
		[ACTION_SUB]          = (f_data_func)&data_counter_t_pass,
		[ACTION_MULTIPLY]     = (f_data_func)&data_counter_t_pass,
		[ACTION_DIVIDE]       = (f_data_func)&data_counter_t_pass,
		[ACTION_WRITE]        = (f_data_func)&data_counter_t_pass,
	}
};
