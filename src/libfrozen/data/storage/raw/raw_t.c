#include <libfrozen.h>
#include <dataproto.h>
#include <raw_t.h>
#include <enum/format/format_t.h>

static ssize_t data_raw_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = ((raw_t *)data->ptr)->size;
	return 0;
} // }}}
static ssize_t data_raw_getdataptr(data_t *data, fastcall_getdataptr *fargs){ // {{{
	fargs->ptr = ((raw_t *)data->ptr)->ptr;
	return 0;
} // }}}
static ssize_t data_raw_copy(data_t *src, fastcall_copy *fargs){ // {{{
	raw_t                 *dst_data;
	raw_t                 *src_data = ((raw_t *)src->ptr);
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	if( (dst_data = fargs->dest->ptr = malloc(sizeof(raw_t))) == NULL)
		return -ENOMEM;
	
	if( (dst_data->ptr = malloc(src_data->size) ) == NULL){
		free(dst_data);
		return -ENOMEM;
	}
	memcpy(dst_data->ptr, src_data->ptr, src_data->size);
	
	dst_data->size    = src_data->size;
	fargs->dest->type = src->type;
	return 0;
} // }}}
static ssize_t data_raw_alloc(data_t *dst, fastcall_alloc *fargs){ // {{{
	raw_t                 *dst_data;

	if( (dst_data = dst->ptr = calloc(sizeof(raw_t), 1)) == NULL)
		return -ENOMEM;
	
	if( (dst_data->ptr = malloc(fargs->length)) == NULL){
		free(dst_data);
		dst->ptr = NULL;
		return -ENOMEM;
	}
	dst_data->size = fargs->length;
	return 0;
} // }}}
static ssize_t data_raw_free(data_t *data, fastcall_free *fargs){ // {{{
	raw_t                 *raw_data = ((raw_t *)data->ptr);
	
	if(raw_data){
		if(raw_data->ptr)
			free(raw_data->ptr);
		
		free(raw_data);
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
		
		default:
			return -ENOSYS;
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_raw_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	raw_t                  new_data;
	raw_t                 *dst_data = ((raw_t *)dst->ptr);
	
	switch(fargs->format){
		case FORMAT(binary):;
			fastcall_read r_read1 = { { 5, ACTION_READ }, 0, &new_data.size, sizeof(new_data.size) };
			if( (ret = data_query(fargs->src, &r_read1)) < 0)
				return ret;
			
			if(dst_data == NULL){
				fastcall_alloc r_alloc = { { 3, ACTION_ALLOC }, new_data.size };
				if( (ret = data_query(dst, &r_alloc)) < 0)
					return ret;

				dst_data = (raw_t *)dst->ptr;
			}else{
				//if(dst_data->size < new_data.size || dst_data->ptr == NULL){
				if(dst_data->ptr == NULL){
					if( (dst_data->ptr = realloc(dst_data->ptr, new_data.size)) == NULL)
						return -ENOMEM;
					
					dst_data->size = new_data.size;
				}
				if(dst_data->size < new_data.size)
					return -ENOSPC;
			}
			
			fastcall_read r_read2 = { { 5, ACTION_READ }, sizeof(new_data.size), dst_data->ptr, dst_data->size };
			if( (ret = data_query(fargs->src, &r_read2)) < 0)
				return ret;
			
			return 0;
		default:
			break;
	};
	return -ENOSYS;
} // }}}

data_proto_t raw_t_proto = {
	.type          = TYPE_RAWT,
	.type_str      = "raw_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_raw_len,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_raw_len,
		[ACTION_GETDATAPTR]  = (f_data_func)&data_raw_getdataptr,
		[ACTION_COPY]        = (f_data_func)&data_raw_copy,
		[ACTION_ALLOC]       = (f_data_func)&data_raw_alloc,
		[ACTION_FREE]        = (f_data_func)&data_raw_free,
		[ACTION_CONVERT_TO]  = (f_data_func)&data_raw_convert_to,
		[ACTION_CONVERT_FROM]= (f_data_func)&data_raw_convert_from,
	}
};

