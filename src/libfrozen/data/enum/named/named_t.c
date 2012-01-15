#include <libfrozen.h>
#include <named_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>

static list                    named_data        = LIST_INITIALIZER;
static pthread_mutex_t         named_mtx         = PTHREAD_MUTEX_INITIALIZER;

static named_t * named_find(char *name){ // {{{
	named_t               *kp;
	void                  *iter              = NULL;
	
	while( (kp = list_iter_next(&named_data, &iter)) != NULL){
		if(strcasecmp(kp->name, name) == 0)
			break;
	}
	return kp;
} // }}}
static void      named_destroy(named_t *fdata){ // {{{
	pthread_mutex_lock(&named_mtx);
		if(fdata->refs-- == 0){
			list_delete(&named_data, fdata);
			free(fdata->name);
			
			fastcall_free r_free = { { 2, ACTION_FREE } };
			data_query(fdata->data, &r_free);
			
			free(fdata);
		}
	pthread_mutex_unlock(&named_mtx);
} // }}}
static void      named_ref_inc(named_t *fdata){ // {{{
	pthread_mutex_lock(&named_mtx);
		fdata->refs++;
	pthread_mutex_unlock(&named_mtx);
} // }}}
named_t *        named_new(char *name, data_t *data){ // {{{
	named_t               *fdata;
	
	if(name == NULL)
		return NULL;
	
	pthread_mutex_lock(&named_mtx);
		
		if( (fdata = named_find(name)) != NULL){
			if(data != NULL)       // user trying to redefine data
				goto error;
			goto found;
		}
		
		if( (fdata = malloc(sizeof(named_t))) == NULL)
			goto error;
		
		if( (fdata->name = strdup(name)) == NULL){
			free(fdata);
			goto error;
		}
		
		fdata->data = NULL;
		fdata->refs = 0;
		
		list_add(&named_data, fdata);
		
	found:
		if(fdata->data == NULL && data != NULL) // define data for ghost named
			fdata->data = data;
		
		fdata->refs++;
	error:
		
	pthread_mutex_unlock(&named_mtx);
	
	return fdata;
} // }}}

static ssize_t data_named_t_default(data_t *data, fastcall_header *hargs){ // {{{
	named_t               *fdata             = (named_t *)data->ptr;
	
	if(fdata == NULL || fdata->data == NULL) // forbid ghost calls
		return -EINVAL;
	
	return data_query(fdata->data, hargs);
} // }}}
static ssize_t data_named_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	char                   buffer[DEF_BUFFER_SIZE];
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(human):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read) < 0)
				return -EINVAL;
			
			buffer[r_read.buffer_size] = '\0';
			
			if( (dst->ptr = named_new(buffer, NULL)) == NULL)
				return -EFAULT;
			
			return 0;
			
		case FORMAT(hash):;
			hash_t                *config;
			data_t                *name;
			data_t                *data;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if( (name = hash_data_find(config, HK(name))) == NULL)
				return -EINVAL;
			
			if( (data = hash_data_find(config, HK(data))) != NULL){
				data_t        *new_data;
				
				if( (new_data = malloc(sizeof(data_t))) == NULL)
					return -ENOMEM;
				
				new_data->type = data->type;
				new_data->ptr  = NULL;
				
				fastcall_copy r_copy = { { 3, ACTION_COPY }, new_data };
				if( (ret = data_query(data, &r_copy)) < 0)
					return ret;
				
				data = new_data;
			}
			
			fastcall_read r_read2 = { { 5, ACTION_READ }, 0, buffer, sizeof(buffer) - 1 };
			if(data_query(name, &r_read2) < 0)
				return -EINVAL;
			
			buffer[r_read2.buffer_size] = '\0';
			
			if( (dst->ptr = named_new(buffer, data)) == NULL)
				return -EFAULT;
			
			return 0;
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_named_t_copy(data_t *src, fastcall_copy *fargs){ // {{{
	if(src->ptr == NULL || fargs->dest == NULL)
		return -EINVAL;
	
	fargs->dest->ptr = src->ptr;
	named_ref_inc(src->ptr);
	return 0;
} // }}}
static ssize_t data_named_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr == NULL)
		return -EINVAL;
	
	named_destroy((named_t *)data->ptr);
	return 0;
} // }}}

data_proto_t named_t_proto = {
	.type                   = TYPE_NAMEDT,
	.type_str               = "named_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_named_t_default,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_named_t_convert_from,
		[ACTION_COPY]         = (f_data_func)&data_named_t_copy,
		[ACTION_FREE]         = (f_data_func)&data_named_t_free,
	}
};
