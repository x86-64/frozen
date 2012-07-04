#include <libfrozen.h>

#include <enum/format/format_t.h>
#include <enum/hashkey/hashkey_t.h>
#include <core/hash/hash_t.h>

#include <inner_t.h>

ssize_t data_inner_t(data_t *data, data_t storage, data_t key, uintmax_t control){ // {{{
	inner_t               *fdata;
	
	if( (fdata = malloc(sizeof(inner_t))) == NULL)
		return -ENOMEM;
	
	fdata->storage = storage;
	fdata->key     = key;
	fdata->control = control;
	fdata->cached_value = NULL;

	data->type = TYPE_INNERT;
	data->ptr  = fdata;
	return 0;
} // }}}
ssize_t data_inner_t_from_config(data_t *data, config_t *config){ // {{{
	ssize_t                ret;
	data_t                 storage;
	data_t                 key;
	uintmax_t              control           = 0;
	
	hash_holder_consume(ret, storage, config, HK(data));
	if(ret < 0)
		goto error1;
	
	hash_holder_consume(ret, key,     config, HK(key));
	if(ret < 0){
		hash_holder_consume(ret, key, config, HK(control_key));
		if(ret < 0)
			goto error2;
		
		control = 1;
	}
	
	if( (ret = data_inner_t(data, storage, key, control)) < 0)
		goto error3;
	
	return 0;
	
error3:
	data_free(&key);
error2:
	data_free(&storage);
error1:
	return ret;
} // }}}
static ssize_t inner_t_find(data_t *data, data_t **inner, data_t *freeit){ // {{{
	ssize_t                ret;
	inner_t               *fdata             = (inner_t *)data->ptr;
	
	if(fdata->control == 0){
		fastcall_lookup r_lookup = {
			{ 4, ACTION_LOOKUP },
			&fdata->key,
			freeit
		};
		ret    = data_query(&fdata->storage, &r_lookup);
		*inner = freeit;
	}else{
		if(fdata->cached_value == NULL){
			fastcall_control r_control = {
				{ 5, ACTION_CONTROL },
				HK(data),
				&fdata->key,
				NULL
			};
			ret    = data_query(&fdata->storage, &r_control);
			fdata->cached_value = r_control.value;
		}else{
			ret    = 0;
		}
		*inner = fdata->cached_value;
	}
	return ret;
} // }}}

static ssize_t data_inner_t_handler(data_t *data, fastcall_header *hargs){ // {{{
	ssize_t                ret;
	data_t                *innerdata;
	data_t                 freeit            = DATA_VOID;
	
	if( (ret = inner_t_find(data, &innerdata, &freeit)) < 0)
		goto exit;
	
	ret = data_query(innerdata, hargs);
exit:
	data_free(&freeit);
	return ret;
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
	switch(fargs->function){
		case HK(data):;
			helper_control_data_t r_internal_empty[] = {
				{ 0, NULL }
			};
			return helper_control_data(data, fargs, r_internal_empty);
			
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
