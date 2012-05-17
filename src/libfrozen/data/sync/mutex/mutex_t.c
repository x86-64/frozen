#include <libfrozen.h>
#include <mutex_t.h>

#include "enum/format/format_t.h"
#include "core/hash/hash_t.h"

mutex_t *            mutex_t_alloc(data_t *data){ // {{{
	mutex_t                 *fdata;
	
	if( (fdata = malloc(sizeof(mutex_t))) == NULL )
		return NULL;
	
	fdata->data       = data;
	data_set_void(&fdata->freeit);
	pthread_mutex_init(&fdata->mutex, NULL);
	return fdata;
} // }}}
void                 mutex_t_destroy(mutex_t *fdata){ // {{{
	pthread_mutex_destroy(&fdata->mutex);
	data_free(&fdata->freeit);
	free(fdata);
} // }}}

static ssize_t data_mutex_t_handler(data_t *data, fastcall_header *hargs){ // {{{
	ssize_t                   ret;
	mutex_t                  *fdata             = (mutex_t *)data->ptr;
	
	pthread_mutex_lock(&fdata->mutex);
		
		ret = data_query(fdata->data, hargs);
		
	pthread_mutex_unlock(&fdata->mutex);
	return ret;
} // }}}
static ssize_t data_mutex_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	mutex_t               *fdata             = (mutex_t *)dst->ptr;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			data_t                 data;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if(fdata != NULL)          // we already inited - pass request
				break;
			
			hash_holder_consume(ret, data, config, HK(data));
			if(ret != 0)
				return -EINVAL;
			
			if( (fdata = dst->ptr = mutex_t_alloc(&data)) == NULL){
				data_free(&data);
				return -ENOMEM;
			}
			
			fdata->freeit = data;
			fdata->data   = &fdata->freeit;
			return 0;
		
		default:
			break;
	}
	
	return data_mutex_t_handler(dst, (fastcall_header *)fargs);
} // }}}
static ssize_t data_mutex_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr)
		mutex_t_destroy((mutex_t *)data->ptr);
	return 0;
} // }}}

data_proto_t mutex_t_proto = {
	.type                   = TYPE_MUTEXT,
	.type_str               = "mutex_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_PROXY,
	.handler_default        = (f_data_func)&data_mutex_t_handler,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_mutex_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_mutex_t_free,
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
	}
};
