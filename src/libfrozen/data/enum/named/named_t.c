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
			
			data_free(&fdata->data);
			free(fdata);
		}
	pthread_mutex_unlock(&named_mtx);
} // }}}
//static void      named_ref_inc(named_t *fdata){ // {{{
//	pthread_mutex_lock(&named_mtx);
//		fdata->refs++;
//	pthread_mutex_unlock(&named_mtx);
//} // }}}
ssize_t          data_named_t(data_t *data, char *name, data_t child){ // {{{
	named_t               *fdata;
	
	if(name == NULL)
		return -EINVAL;
	
	pthread_mutex_lock(&named_mtx);
		
		if( (fdata = named_find(name)) != NULL){
			if(!data_is_void(&child))       // user trying to redefine data
				goto error;
			goto found;
		}
		
		if( (fdata = malloc(sizeof(named_t))) == NULL)
			goto error;
		
		if( (fdata->name = strdup(name)) == NULL){
			free(fdata);
			goto error;
		}
		
		data_set_void(&fdata->data);
		fdata->refs = 0;
		
		list_add(&named_data, fdata);
		
	found:
		if(data_is_void(&fdata->data)){
			if(! (data_is_void(&child)) ) // define child for ghost named
				fdata->data = child;
		}
		
		fdata->refs++;
	error:
		
	pthread_mutex_unlock(&named_mtx);
	
	data->type = TYPE_NAMEDT;
	data->ptr  = fdata;
	return fdata ? 0 : -EFAULT;
} // }}}

static ssize_t data_named_t_default(data_t *data, fastcall_header *hargs){ // {{{
	named_t               *fdata             = (named_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
	return data_query(&fdata->data, hargs);
} // }}}
static ssize_t data_named_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	data_t                 d_void            = DATA_VOID;
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
			
			return data_named_t(dst, buffer, d_void);
			
		case FORMAT(hash):;
			hash_t                *config;
			data_t                *name;
			data_t                 data           = DATA_VOID;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if( (name = hash_data_find(config, HK(name))) == NULL)
				return -EINVAL;
			
			fastcall_read r_read2 = { { 5, ACTION_READ }, 0, buffer, sizeof(buffer) - 1 };
			if(data_query(name, &r_read2) < 0)
				return -EINVAL;
			
			buffer[r_read2.buffer_size] = '\0';
			
			hash_holder_consume(ret, data, config, HK(data));
			
			return data_named_t(dst, buffer, data);
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_named_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr == NULL)
		return 0;
	
	named_destroy((named_t *)data->ptr);
	return 0;
} // }}}

data_proto_t named_t_proto = {
	.type                   = TYPE_NAMEDT,
	.type_str               = "named_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_PROXY,
	.handler_default        = (f_data_func)&data_named_t_default,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_named_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_named_t_free,
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
	}
};
