#include <libfrozen.h>

#include <enum/format/format_t.h>
#include <enum/hashkey/hashkey_t.h>
#include <core/hash/hash_t.h>

#include <env_t.h>

ssize_t data_env_t(data_t *data, hashkey_t key){ // {{{
	env_t                 *fdata;

	if( (fdata = malloc(sizeof(env_t))) == NULL)
		return -ENOMEM;
	
	fdata->key = key;
	
	data->type = TYPE_ENVT;
	data->ptr  = fdata;
	return 0;
} // }}}
static ssize_t  data_env_t_from_config(data_t *data, config_t *config){ // {{{
	ssize_t                ret;
	hashkey_t              key;

	hash_data_get(ret, TYPE_HASHKEYT, key, config, HK(key));
	if(ret < 0)
		return ret;
	
	return data_env_t(data, key);
} // }}}
static ssize_t  data_env_t_from_string(data_t *data, data_t *string){ // {{{
	ssize_t                ret;
	hashkey_t              key;
	data_t                 d_key             = DATA_PTR_HASHKEYT(&key);
	
	fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, string, FORMAT(human) };
	if( (ret = data_query(&d_key, &r_convert)) < 0 )
		return ret;
	
	return data_env_t(data, key);
} // }}}
static void     data_env_t_destroy(data_t *data){ // {{{
	env_t                 *fdata             = (env_t *)data->ptr;
	
	if(fdata){
		free(fdata);
	}
	data_set_void(data);
} // }}}

static data_t * data_env_t_find(env_t *fdata){ // {{{
	list                  *stack;
	request_t             *curr;
	data_t                *data              = NULL;
	void                  *iter              = NULL;
	
	if(fdata == NULL)
		return NULL;
	
	if( (stack = request_get_stack()) == NULL)
		return NULL;
	
	while( (curr = list_iter_next(stack, &iter)) != NULL){
		if( (data = hash_data_find(curr, fdata->key)) != NULL)
			break;
	}
	
	return data;
} // }}}

static ssize_t data_env_t_handler(data_t *data, fastcall_header *hargs){ // {{{
	data_t                *rdata;
	env_t                 *fdata             = (env_t *)data->ptr;
	
	if( (rdata = data_env_t_find(fdata)) == NULL)
		return -EINVAL;
	
	return data_query(rdata, hargs);
} // }}}
static ssize_t data_env_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(dst->ptr != NULL)
		return data_env_t_handler(dst, (fastcall_header *)fargs);  // already inited - pass to underlying data
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return data_env_t_from_config(dst, config);
		
		case FORMAT(config):;
		case FORMAT(human):;
			return data_env_t_from_string(dst, fargs->src);
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_env_t_free(data_t *data, fastcall_free *hargs){ // {{{
	data_env_t_destroy(data);
	return 0;
} // }}}

data_proto_t env_t_proto = {
	.type_str               = "env_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_env_t_handler,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_env_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_env_t_free,
	}
};
