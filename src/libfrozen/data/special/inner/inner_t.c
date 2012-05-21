#include <libfrozen.h>

#include <enum/format/format_t.h>
#include <enum/hashkey/hashkey_t.h>
#include <core/hash/hash_t.h>

#include <inner_t.h>

ssize_t data_inner_t(data_t *data, data_t storage, data_t key){ // {{{
	inner_t               *fdata;

	if( (fdata = malloc(sizeof(inner_t))) == NULL)
		return -ENOMEM;
	
	fdata->storage = storage;
	fdata->key     = key;
	
	data->type = TYPE_INNERT;
	data->ptr  = fdata;
	return 0;
} // }}}
ssize_t data_inner_t_from_config(data_t *data, config_t *config){ // {{{
	ssize_t                ret;
	data_t                 storage;
	data_t                 key;
	
	hash_holder_consume(ret, storage, config, HK(data));
	if(ret < 0)
		goto error1;
	
	hash_holder_consume(ret, key,     config, HK(key));
	if(ret < 0)
		goto error2;
	
	if( (ret = data_inner_t(data, storage, key)) < 0)
		goto error3;
	
	return 0;
	
error3:
	data_free(&key);
error2:
	data_free(&storage);
error1:
	return ret;
} // }}}
static ssize_t inner_t_find(data_t *data, data_t **inner){ // {{{
	ssize_t                ret;
	inner_t               *fdata             = (inner_t *)data->ptr;
	
	fastcall_control r_control = {
		{ 5, ACTION_CONTROL },
		HK(data),
		&fdata->key,
		NULL
	};
	ret    = data_query(&fdata->storage, &r_control);
	*inner = r_control.value;
	return ret;
} // }}}

static ssize_t data_inner_t_handler(data_t *data, fastcall_header *hargs){ // {{{
	ssize_t                ret;
	data_t                *innerdata;
	
	if( (ret = inner_t_find(data, &innerdata)) < 0)
		return ret;
	
	return data_query(innerdata, hargs);
} // }}}
static ssize_t data_inner_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                  ret;
	
	if(dst->ptr != NULL)
		return data_inner_t_handler(dst, (fastcall_header *)fargs);  // already inited - pass to underlying data
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return data_inner_t_from_config(dst, config);
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_inner_t_free(data_t *data, fastcall_free *hargs){ // {{{
	inner_t               *fdata             = (inner_t *)data->ptr;
	
	if(fdata){
		data_free(&fdata->key);
		data_free(&fdata->storage);
		free(fdata);
	}
	data_set_void(data);
	return 0;
} // }}}
static ssize_t data_inner_t_control(data_t *data, fastcall_control *fargs){ // {{{
	data_t                *innerdata;
	
	switch(fargs->function){
		case HK(data):;
			if(inner_t_find(data, &innerdata) < 0){
				helper_control_data_t r_internal_empty[] = {
					{ 0, NULL }
				};
				return helper_control_data(data, fargs, r_internal_empty);
			}else{
				helper_control_data_t r_internal[] = {
					{ HK(data), innerdata },
					{ 0, NULL }
				};
				return helper_control_data(data, fargs, r_internal);
			}
			break;
			
		default:
			break;
	}
	return data_default_control(data, fargs);
} // }}}

data_proto_t inner_t_proto = {
	.type_str               = "inner_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_inner_t_handler,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_inner_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_inner_t_free,
		[ACTION_CONTROL]      = (f_data_func)&data_inner_t_control,
	}
};
