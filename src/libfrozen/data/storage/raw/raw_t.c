#include <libfrozen.h>
#include <raw_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>

static void    raw_freeptr(data_t *data){ // {{{
	raw_t                 *fdata             = ((raw_t *)data->ptr);
	
	if( (fdata->flags & RAW_ONECHUNK) == 0){ // free only if chunk was allocated separately 
		if(fdata->ptr)
			free(fdata->ptr);
	}
	fdata->ptr  = NULL;
	fdata->size = 0;
} // }}}
static ssize_t raw_prepare(data_t *data, uintmax_t new_size){ // {{{
	raw_t                 *fdata             = (raw_t *)data->ptr;
	
	if(data->type != TYPE_RAWT || __MAX(uintmax_t) - sizeof(raw_t) <= new_size)
		return -EINVAL;
	
	if( (fdata = data->ptr) == NULL){ // empty data
		if( (fdata = data->ptr = malloc(sizeof(raw_t) + new_size)) == NULL)
			return -ENOMEM;
		
		fdata->ptr   = new_size == 0 ? NULL : (void *)(fdata + 1);
		fdata->size  = new_size;
		fdata->flags = RAW_RESIZEABLE | RAW_ONECHUNK;
		return 0;
	}
	
	if(new_size == 0){ // request was to free data
		raw_freeptr(data);
		return 0;
	}
	
	if(fdata->ptr == NULL){ // empty data holder
		if( (fdata->ptr = malloc(new_size)) == NULL)
			return -ENOMEM;
		
		fdata->size   = new_size;
		fdata->flags |= RAW_RESIZEABLE;
		return 0;
	}
	
	if(fdata->size >= new_size) // is enough space?
		return 0;
	
	if( (fdata->flags & RAW_RESIZEABLE) != 0){ // can resize?
		if( (fdata->flags & RAW_ONECHUNK) != 0){
			if( (fdata = data->ptr = realloc(fdata, sizeof(raw_t) + new_size)) == NULL)
				return -ENOMEM;
		}else{
			if( (fdata->ptr = realloc(fdata->ptr, new_size)) == NULL){
				fdata->size = 0;
				return -ENOMEM;
			}
		}
		fdata->size = new_size;
		return 0;
	}
	
	return -EINVAL; // failed to prepare data
} // }}}
static ssize_t raw_read(data_t *dst, data_t *src, uintmax_t offset, uintmax_t *length){ // {{{
	ssize_t                ret, ret2;
	raw_t                 *dst_data;
	
	switch( (ret = raw_prepare(dst, *length)) ){
		case 0:       break;
		case -ENOSPC: break;
		default:      return ret;
	}
	dst_data = (raw_t *)(dst->ptr);
	
	if(src && *length > 0){
		fastcall_read r_read = {
			{ 5, ACTION_READ },
			offset,
			dst_data->ptr,
			MIN(dst_data->size, *length)
		};
		if( (ret2 = data_query(src, &r_read)) < 0)
			return ret2;
		
		*length = r_read.buffer_size;
	}
	return ret;
} // }}}

raw_t *        raw_t_alloc(uintmax_t size){ // {{{
	data_t                 new_raw           = DATA_RAWT_EMPTY();
	
	if(raw_prepare(&new_raw, size) < 0)
		return NULL;
	
	return new_raw.ptr;
} // }}}
void           data_raw_t_destroy(data_t *data){ // {{{
	raw_t                 *fdata             = ((raw_t *)data->ptr);
	
	if(fdata){
		raw_freeptr(data);
		free(fdata);
	}
	data_set_void(data);
} // }}}

static ssize_t data_raw_len(data_t *data, fastcall_length *fargs){ // {{{
	raw_t                 *fdata = ((raw_t *)data->ptr);
	
	fargs->length = fdata ? fdata->size : 0;
	return 0;
} // }}}
static ssize_t data_raw_view(data_t *data, fastcall_view *fargs){ // {{{
	raw_t                 *fdata             = ((raw_t *)data->ptr);
	
	if(fdata){
		fargs->ptr    = fdata->ptr;  
		fargs->length = fdata->size;
		data_set_void(&fargs->freeit);
		return 0;
	}
	return -EINVAL;
} // }}}
static ssize_t data_raw_free(data_t *data, fastcall_free *fargs){ // {{{
	data_raw_t_destroy(data);
	return 0;
} // }}}

static ssize_t data_raw_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              transfered        = 0;
	raw_t                 *src_data          = ((raw_t *)src->ptr);
	
	if(fargs->dest == NULL || !src_data)
		return -EINVAL;
	
	switch(fargs->format){
		case FORMAT(native):;
		default:;
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, src_data->ptr, src_data->size };
			ret = data_query(fargs->dest, &r_write);
			
			transfered = r_write.buffer_size;
			
			break;
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_raw_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              length            = 0;

	switch(fargs->format){
		case FORMAT(native):
		case FORMAT(human):
		case FORMAT(config):;
		default:;
			
			fastcall_length r_len1 = { { 4, ACTION_LENGTH }, 0, FORMAT(native) };
			if( (ret = data_query(fargs->src, &r_len1)) < 0)
				return ret;
			
			length = r_len1.length;
			
			ret = raw_read(dst, fargs->src, 0, &length);
			break;
			
		case FORMAT(hash):;
			hash_t                *config;
			data_t                *buffer;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			buffer = hash_data_find(config, HK(buffer));
			if(buffer){
				fastcall_length r_len2 = { { 4, ACTION_LENGTH }, 0, FORMAT(native) };
				if( (ret = data_query(buffer, &r_len2)) < 0)
					return ret;
				
				length = r_len2.length;
			}
			hash_data_get(ret, TYPE_UINTT, length, config, HK(length));
			
			ret = raw_read(dst, buffer, 0, &length);
			break;
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = length;
	
	return ret;
} // }}}
static ssize_t data_raw_write(data_t *data, fastcall_write *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              new_size;
	raw_t                 *fdata;
	
	if(fargs->buffer_size == 0)
		return 0;
	
	if(__MAX(uintmax_t) - fargs->buffer_size <= fargs->offset)
		return -EINVAL;
	
	new_size = fargs->offset + fargs->buffer_size; 
	
	switch( (ret = raw_prepare(data, new_size))){
		case 0:       break;
		case -EINVAL: break;
		default:      return ret;
	}

	fdata = (raw_t *)(data->ptr);
	
	fargs->buffer_size = MIN(fargs->buffer_size, fdata->size - fargs->offset);
	
	if(fargs->buffer_size == 0)
		return -1; // EOF
	
	memcpy(fdata->ptr + fargs->offset, fargs->buffer, fargs->buffer_size);
	return 0;
} // }}}
static ssize_t data_raw_resize(data_t *data, fastcall_resize *fargs){ // {{{
	return raw_prepare(data, fargs->length);
} // }}}

data_proto_t raw_t_proto = {
	.type          = TYPE_RAWT,
	.type_str      = "raw_t",
	.api_type      = API_HANDLERS,
	.properties    = DATA_GREEDY | DATA_ENDPOINT,
	.handlers      = {
		[ACTION_LENGTH]      = (f_data_func)&data_raw_len,
		[ACTION_VIEW]        = (f_data_func)&data_raw_view,
		[ACTION_FREE]        = (f_data_func)&data_raw_free,
		[ACTION_CONVERT_TO]  = (f_data_func)&data_raw_convert_to,
		[ACTION_CONVERT_FROM]= (f_data_func)&data_raw_convert_from,
		[ACTION_WRITE]       = (f_data_func)&data_raw_write,
		[ACTION_RESIZE]      = (f_data_func)&data_raw_resize,
	}
};

