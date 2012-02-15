#include <libfrozen.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>

#include <ref_t.h>

ref_t *           ref_t_alloc(data_t *data){ // {{{
	ref_t                 *fdata;

	if( (fdata = malloc(sizeof(ref_t))) == NULL )
		return NULL;
	
	fdata->refs = 1;
	fdata->data = *data;
	return fdata;
} // }}}
void              ref_t_acquire(ref_t *ref){ // {{{
	ref->refs++;
} // }}}
void              ref_t_destroy(ref_t *ref){ // {{{
	if(ref->refs-- == 1){
		data_free(&ref->data);
	}
} // }}}

static ssize_t data_ref_t_handler(data_t *data, fastcall_header *hargs){ // {{{
	ref_t                 *fdata             = (ref_t *)data->ptr;
	
	return data_query(&fdata->data, hargs);
} // }}}
static ssize_t data_ref_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(dst->ptr != NULL)
		return data_ref_t_handler(dst, (fastcall_header *)fargs);  // already inited - pass to underlying data
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			data_t                 data;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			hash_holder_consume(ret, data, config, HK(data));
			
			return ( (dst->ptr = ref_t_alloc(&data)) == NULL ) ? -EFAULT : 0;

		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_ref_t_free(data_t *data, fastcall_free *hargs){ // {{{
	ref_t                 *fdata             = (ref_t *)data->ptr;
	
	ref_t_destroy(fdata);
	return 0;
} // }}}
static ssize_t data_ref_t_acquire(data_t *data, fastcall_acquire *hargs){ // {{{
	ref_t                 *fdata             = (ref_t *)data->ptr;
	
	ref_t_acquire(fdata);
	return 0;
} // }}}

data_proto_t ref_t_proto = {
	.type_str               = "ref_t",
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_ref_t_handler,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_ref_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_ref_t_free,
		[ACTION_ACQUIRE]      = (f_data_func)&data_ref_t_acquire,
	}
};
