#include <libfrozen.h>
#include <raw_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>

static ssize_t raw_resize(raw_t *fdata, uintmax_t new_size){ // {{{
	if( (fdata->flags & RAW_RESIZEABLE) == 0)
		return -EINVAL;
	
	if(new_size != 0){
		if( (fdata->ptr = realloc(fdata->ptr, new_size)) == NULL){
			fdata->size = 0;
			return -ENOMEM;
		}
	}else{
		if(fdata->ptr)
			free(fdata->ptr);
		fdata->ptr = NULL;
	}
	fdata->size = new_size;
	return 0;
} // }}}
static ssize_t raw_prepare(data_t *data, uintmax_t new_size){ // {{{
	raw_t                 *raw_data;
	
	if(data == NULL || data->type != TYPE_RAWT)
		return -EINVAL;
	
	if( (raw_data = data->ptr) == NULL){ // empty data
		if( (raw_data = data->ptr = malloc(sizeof(raw_t))) == NULL)
			return -ENOMEM;
		
		if(new_size != 0){
			if( (raw_data->ptr = malloc(new_size)) == NULL){
				free(raw_data);
				data->ptr = NULL;
				return -ENOMEM;
			}
		}else{
			raw_data->ptr = NULL;
		}
		
		raw_data->size   = new_size;
		raw_data->flags |= RAW_RESIZEABLE;
		return 0;
	}
	
	if(raw_data->ptr == NULL){ // empty data holder
		if(new_size != 0){
			if( (raw_data->ptr = malloc(new_size)) == NULL)
				return -ENOMEM;
		}else{
			raw_data->ptr = NULL;
		}
		
		raw_data->size   = new_size;
		raw_data->flags |= RAW_RESIZEABLE;
		return 0;
	}
	
	if(raw_data->size < new_size){ // too less space?
		if(raw_resize(raw_data, new_size) < 0)
			return -ENOSPC;
	}
	
	return 0;
} // }}}
static ssize_t raw_read(data_t *dst, data_t *src, uintmax_t offset, uintmax_t length){ // {{{
	ssize_t                ret, ret2;
	raw_t                 *dst_data;
	
	switch( (ret = raw_prepare(dst, length)) ){
		case 0:       break;
		case -ENOSPC: break;
		default:      return ret;
	}
	dst_data = (raw_t *)(dst->ptr);
	
	if(src && length > 0){
		fastcall_read r_read = {
			{ 5, ACTION_READ },
			offset,
			dst_data->ptr,
			MIN(dst_data->size, length)
		};
		if( (ret2 = data_query(src, &r_read)) < 0)
			return ret2;
		
	}
	return ret;
} // }}}

static ssize_t data_raw_len(data_t *data, fastcall_length *fargs){ // {{{
	fargs->length = ((raw_t *)data->ptr)->size;
	switch(fargs->format){
		case FORMAT(clean):
		case FORMAT(human):
		case FORMAT(config):
			break;
		case FORMAT(binary):
			fargs->length += sizeof(((raw_t *)data->ptr)->size);
			break;
		default:
			return -ENOSYS;
	}
	return 0;
} // }}}
static ssize_t data_raw_getdataptr(data_t *data, fastcall_getdataptr *fargs){ // {{{
	fargs->ptr = ((raw_t *)data->ptr)->ptr;
	return 0;
} // }}}
static ssize_t data_raw_copy(data_t *src, fastcall_copy *fargs){ // {{{
	ssize_t                ret;
	raw_t                 *dst_data;
	raw_t                 *src_data = ((raw_t *)src->ptr);
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	fargs->dest->type = TYPE_RAWT;
	fargs->dest->ptr  = NULL;
	
	switch( (ret = raw_prepare(fargs->dest, src_data->size)) ){
		case 0:       break;
		case -ENOSPC: break;
		default:      return ret;
	}
	
	dst_data = (raw_t *)(fargs->dest->ptr);
	
	memcpy(dst_data->ptr, src_data->ptr, MIN(dst_data->size, src_data->size));
	return ret;
} // }}}
static ssize_t data_raw_alloc(data_t *dst, fastcall_alloc *fargs){ // {{{
	if(dst->ptr != NULL)        // accept only empty data, in other case it could lead to memleak
		return -EEXIST;
	
	return raw_prepare(dst, fargs->length);
} // }}}
static ssize_t data_raw_free(data_t *data, fastcall_free *fargs){ // {{{
	raw_t                 *raw_data = ((raw_t *)data->ptr);
	
	if(raw_data){
		if(raw_data->ptr)
			free(raw_data->ptr);
		
		free(raw_data);
		data->ptr = NULL;
	}
	return 0;
} // }}}
static ssize_t data_raw_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              transfered        = 0;
	raw_t                 *src_data          = ((raw_t *)src->ptr);
	
	if(fargs->dest == NULL || !src_data)
		return -EINVAL;
	
	switch(fargs->format){
		case FORMAT(clean):;
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, src_data->ptr, src_data->size };
			ret = data_query(fargs->dest, &r_write);
			
			transfered = r_write.buffer_size;
			
			break;

		case FORMAT(binary):;
			fastcall_write r_write1 = { { 5, ACTION_WRITE }, 0,                     &src_data->size, sizeof(src_data->size) };
			ret         = data_query(fargs->dest, &r_write1);
			transfered += r_write1.buffer_size;
			
			if(ret < 0)
				break;
			
			fastcall_write r_write2 = { { 5, ACTION_WRITE }, sizeof(src_data->size), src_data->ptr,  src_data->size };
			ret         = data_query(fargs->dest, &r_write2);
			transfered += r_write2.buffer_size;
	
			break;
		
		case FORMAT(netstring):;
			char                   buffer[100];
			intmax_t               buffer_size;
			
			if( (buffer_size = snprintf(buffer, sizeof(buffer), "%" SCNuMAX ":", src_data->size)) <= 0)
				return -EINVAL;
			
			fastcall_write r_net_write1 = { { 5, ACTION_WRITE }, 0,            buffer,         buffer_size };
			ret         = data_query(fargs->dest, &r_net_write1);
			transfered += r_net_write1.buffer_size;	
			if(ret < 0) break;
			
			fastcall_write r_net_write2 = { { 5, ACTION_WRITE }, buffer_size,  src_data->ptr,  src_data->size };
			ret         = data_query(fargs->dest, &r_net_write2);
			transfered += r_net_write2.buffer_size;
			if(ret < 0) break;
			
			fastcall_write r_net_write3 = { { 5, ACTION_WRITE }, buffer_size + src_data->size, ",", 1 };
			ret         = data_query(fargs->dest, &r_net_write3);
			transfered += r_net_write3.buffer_size;
			
			break;
		default:
			return -ENOSYS;
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_raw_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              length            = 0;
	
	switch(fargs->format){
		case FORMAT(clean):
		case FORMAT(human):
		case FORMAT(config):;
			
			fastcall_length r_len1 = { { 4, ACTION_LENGTH }, 0, FORMAT(clean) };
			if( (ret = data_query(fargs->src, &r_len1)) < 0)
				return ret;
			
			return raw_read(dst, fargs->src, 0, r_len1.length);
			
		case FORMAT(hash):;
			hash_t                *config;
			data_t                *buffer;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			buffer = hash_data_find(config, HK(buffer));
			if(buffer){
				fastcall_length r_len2 = { { 4, ACTION_LENGTH }, 0, FORMAT(clean) };
				if( (ret = data_query(buffer, &r_len2)) < 0)
					return ret;
				
				length = r_len2.length;
			}
			hash_data_get(ret, TYPE_UINTT, length, config, HK(length));
			
			return raw_read(dst, buffer, 0, length);

		case FORMAT(binary):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &length, sizeof(length) };
			if( (ret = data_query(fargs->src, &r_read)) < 0)
				return ret;
			
			return raw_read(dst, fargs->src, sizeof(length), length);
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_raw_write(data_t *dst, fastcall_write *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              new_size;
	raw_t                 *fdata;
	
	if(fargs->buffer_size == 0)
		return 0;
	
	if(fargs->buffer == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){
		if( (ret = raw_prepare(dst, 0)) < 0)
			return ret;
	}
	fdata = dst->ptr;
	
	// check if we need to resize it?
	
	if(__MAX(uintmax_t) - fargs->buffer_size <= fargs->offset)
		return -EINVAL;
	
	new_size = fargs->offset + fargs->buffer_size; 
	if(fdata->size < new_size){
		switch( (ret = raw_resize(fdata, new_size))){
			case 0:       break;
			case -EINVAL: goto not_resizeable;
			default:      return ret;
		}
	}

not_resizeable:
	fargs->buffer_size = MIN(fargs->buffer_size, fdata->size - fargs->offset);
	
	if(fargs->buffer_size == 0)
		return -1; // EOF
	
	memcpy(fdata->ptr + fargs->offset, fargs->buffer, fargs->buffer_size);
	return 0;
} // }}}
static ssize_t data_raw_resize(data_t *data, fastcall_resize *fargs){ // {{{
	raw_t                 *fdata             = ((raw_t *)data->ptr);
	
	if(fdata == NULL)
		return raw_prepare(data, fargs->length);
	
	return raw_resize(fdata, fargs->length);
} // }}}

data_proto_t raw_t_proto = {
	.type          = TYPE_RAWT,
	.type_str      = "raw_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_LENGTH]      = (f_data_func)&data_raw_len,
		[ACTION_GETDATAPTR]  = (f_data_func)&data_raw_getdataptr,
		[ACTION_COPY]        = (f_data_func)&data_raw_copy,
		[ACTION_ALLOC]       = (f_data_func)&data_raw_alloc,
		[ACTION_FREE]        = (f_data_func)&data_raw_free,
		[ACTION_CONVERT_TO]  = (f_data_func)&data_raw_convert_to,
		[ACTION_CONVERT_FROM]= (f_data_func)&data_raw_convert_from,
		[ACTION_WRITE]       = (f_data_func)&data_raw_write,
		[ACTION_RESIZE]      = (f_data_func)&data_raw_resize,
	}
};

