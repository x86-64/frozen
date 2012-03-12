#include <libfrozen.h>

#include <enum/format/format_t.h>
#include <enum/hashkey/hashkey_t.h>
#include <core/hash/hash_t.h>

#include <env_t.h>

static data_t * env_t_find(env_t *fdata){ // {{{
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
	
	if( (rdata = env_t_find(fdata)) == NULL)
		return -EINVAL;
	
	return data_query(rdata, hargs);
} // }}}
static ssize_t data_env_t_alloc(data_t *data, fastcall_alloc *hargs){ // {{{
	if( (data->ptr = malloc(sizeof(env_t))) == NULL)
		return -ENOMEM;
	
	((env_t *)data->ptr)->key = HK(input);
	return 0;
} // }}}
static ssize_t data_env_t_free(data_t *data, fastcall_free *hargs){ // {{{
	free(data->ptr);
	data->ptr = NULL;
	return 0;
} // }}}
/*static ssize_t data_env_t_copy(data_t *src, fastcall_copy *fargs){ // {{{
	env_t                 *fdata             = (env_t *)src->ptr;
	env_t                 *new_fdata         = NULL;
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	if(fdata){
		if( (new_fdata = malloc(sizeof(env_t))) == NULL)
			return -EINVAL;
		
		new_fdata->key = fdata->key;
	}
	fargs->dest->type = src->type;
	fargs->dest->ptr  = new_fdata;
	return 0;
} // }}}*/
static ssize_t data_env_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	env_t                 *fdata;
	
	if(dst->ptr != NULL)
		return data_env_t_handler(dst, (fastcall_header *)fargs);  // already inited - pass to underlying data
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if( (ret = data_env_t_alloc(dst, NULL)) != 0)
		return ret;
	
	fdata = (env_t *)dst->ptr;
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(human):;
			data_t                 hkey              = DATA_PTR_HASHKEYT(&fdata->key);
			
			fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, fargs->src, fargs->format };
			return data_query(&hkey, &r_convert);
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_env_t_getdata(data_t *data, fastcall_getdata *fargs){ // {{{
	env_t                 *fdata             = (env_t *)data->ptr;
	
	if( (fargs->data = env_t_find(fdata)) == NULL)
		return -EINVAL;
	
	return 0;
} // }}}

data_proto_t env_t_proto = {
	.type_str               = "env_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_env_t_handler,
	.handlers               = {
		[ACTION_ALLOC]        = (f_data_func)&data_env_t_alloc,
		//[ACTION_COPY]         = (f_data_func)&data_env_t_copy,
		[ACTION_FREE]         = (f_data_func)&data_env_t_free,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_env_t_convert_from,
		[ACTION_GETDATA]      = (f_data_func)&data_env_t_getdata,
	}
};
