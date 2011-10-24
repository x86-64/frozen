#include <libfrozen.h>
#include <dataproto.h>

#include <string/string_t.h>

static ssize_t       data_default_read          (data_t *data, fastcall_read *fargs){ // {{{
	fastcall_physicallen r_len = { { 3, ACTION_LOGICALLEN } };
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(data, &r_len) != 0 || data_query(data, &r_ptr) != 0 || r_ptr.ptr == NULL)
		return -EFAULT;
	
	if(r_len.length == 0)
		return -1; // EOF
	
	if(fargs->buffer == NULL || fargs->offset > r_len.length)
		return -EINVAL; // invalid range
	
	fargs->buffer_size = MIN(fargs->buffer_size, r_len.length - fargs->offset);
	
	if(fargs->buffer_size == 0)
		return -1; // EOF
	
	memcpy(fargs->buffer, r_ptr.ptr + fargs->offset, fargs->buffer_size);
	return 0;
} // }}}
static ssize_t       data_default_write         (data_t *data, fastcall_write *fargs){ // {{{
	fastcall_physicallen r_len = { { 3, ACTION_LOGICALLEN } };
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(data, &r_len) != 0 || data_query(data, &r_ptr) != 0 || r_ptr.ptr == NULL)
		return -EFAULT;
	
	if(r_len.length == 0)
		return -1; // EOF
	
	if(fargs->buffer == NULL || fargs->offset > r_len.length)
		return -EINVAL; // invalid range
	
	fargs->buffer_size = MIN(fargs->buffer_size, r_len.length - fargs->offset);
	
	if(fargs->buffer_size == 0)
		return -1; // EOF
	
	memcpy(r_ptr.ptr + fargs->offset, fargs->buffer, fargs->buffer_size);
	return 0;
} // }}}
static ssize_t       data_default_copy          (data_t *src, fastcall_copy *fargs){ // {{{
	fastcall_getdataptr r_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(src, &r_ptr) != 0)
		return -EFAULT;
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	if(r_ptr.ptr != NULL){
		fastcall_physicallen r_len = { { 3, ACTION_PHYSICALLEN } };
		if( data_query(src, &r_len) != 0)
			return -EFAULT;
		
		if( (fargs->dest->ptr = malloc(r_len.length)) == NULL)
			return -EFAULT;
		
		memcpy(fargs->dest->ptr, r_ptr.ptr, r_len.length);
	}else{
		fargs->dest->ptr = NULL;
	}
	
	fargs->dest->type = src->type;
	return 0;
} // }}}
static ssize_t       data_default_compare       (data_t *data1, fastcall_compare *fargs){ // {{{
	ssize_t                cret;
	
	if(fargs->data2 == NULL)
		return -EINVAL;
	
	fastcall_physicallen r1_len = { { 3, ACTION_PHYSICALLEN } };
	fastcall_getdataptr  r1_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(data1, &r1_len) != 0 || data_query(data1, &r1_ptr) != 0 || r1_ptr.ptr == NULL)
		return -EFAULT;
	
	fastcall_physicallen r2_len = { { 3, ACTION_PHYSICALLEN } };
	fastcall_getdataptr  r2_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(fargs->data2, &r2_len) != 0 || data_query(fargs->data2, &r2_ptr) != 0 || r2_ptr.ptr == NULL)
		return -EFAULT;
	
	     if(r1_len.length > r2_len.length){ cret =  1; }
	else if(r1_len.length < r2_len.length){ cret = -1; }
	else {
		cret = memcmp(r1_ptr.ptr, r2_ptr.ptr, r1_len.length);
		if(cret > 0) cret =  1;
		if(cret < 0) cret = -1;
	}
	return cret;
/*int                  data_cmp               (data_t *data1, data_ctx_t *data1_ctx, data_t *data2, data_ctx_t *data2_ctx){
	char            buffer1_local[DEF_BUFFER_SIZE], buffer2_local[DEF_BUFFER_SIZE];
	void           *buffer1, *buffer2;
	size_t          buffer1_size, buffer2_size, cmp_size;
	ssize_t         ret;
	off_t           offset1, offset2, goffset1, goffset2;
	f_data_cmp      func_cmp;
	
	if(!data_validate(data1) || !data_validate(data2))
		return -EINVAL;
	
	if(data1->type != data2->type){
		
		goffset1     = goffset2     = 0;
		buffer1_size = buffer2_size = 0;
		do {
			if(buffer1_size == 0){
				buffer1      = buffer1_local;
				buffer1_size = DEF_BUFFER_SIZE;
				
				if( (ret = data_read_raw  (data1, data1_ctx, goffset1, &buffer1, &buffer1_size)) < -1)
					return -EINVAL;
				
				if(ret == -1 && buffer2_size != 0)
					return 1;
				
				goffset1 += buffer1_size;
				offset1 = 0;
			}
			if(buffer2_size == 0){
				buffer2      = buffer2_local;
				buffer2_size = DEF_BUFFER_SIZE;
				
				if( (ret = data_read_raw  (data2, data2_ctx, goffset2, &buffer2, &buffer2_size)) < -1)
					return -EINVAL;
				
				if(ret == -1)
					return (buffer1_size == 0) ? 0 : -1;
				
				goffset2 += buffer2_size;
				offset2   = 0;
			}
			
			cmp_size = (buffer1_size < buffer2_size) ? buffer1_size : buffer2_size;
			
			ret = memcmp(buffer1 + offset1, buffer2 + offset2, cmp_size);
			if(ret != 0)
				return ret;
			
			offset1      += cmp_size;
			offset2      += cmp_size;
			buffer1_size -= cmp_size;
			buffer2_size -= cmp_size;
		}while(1);
	}
	
	if( (func_cmp = data_protos[data1->type]->func_cmp) == NULL)
		return -ENOSYS;
	
	return func_cmp(data1, data1_ctx, data2, data2_ctx);
} */
} // }}}
static ssize_t       data_default_transfer      (data_t *src, fastcall_transfer *fargs){ // {{{
	char            buffer[DEF_BUFFER_SIZE];
	ssize_t         rret, wret;
	uintmax_t       roffset, woffset;
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	roffset = woffset = 0;
	do {
		fastcall_read r_read = { { 5, ACTION_READ }, roffset, &buffer, sizeof(buffer) };
		if( (rret = data_query(src, &r_read)) < -1)
			return rret;
		
		if(rret == -1) // EOF from read side
			break;
		
		fastcall_write r_write = { { 5, ACTION_WRITE }, woffset, &buffer, r_read.buffer_size };
		if( (wret = data_query(fargs->dest, &r_write)) < -1)
			return wret;
		
		roffset += r_read.buffer_size;
		woffset += r_write.buffer_size;
		
		if(wret == -1) // EOF from write side
			break;
	}while(1);
	return 0;
} // }}}
static ssize_t       data_default_free          (data_t *data, fastcall_free *fargs){ // {{{
	fastcall_getdataptr  r_ptr = { { 3, ACTION_GETDATAPTR } };
	if( data_query(data, &r_ptr) != 0 || r_ptr.ptr == NULL)
		return -EFAULT;
	
	if(r_ptr.ptr != NULL)
		free(r_ptr.ptr);
	return 0;
} // }}}
static ssize_t       data_default_getdataptr    (data_t *data, fastcall_getdataptr *fargs){ // {{{
	fargs->ptr = data->ptr;
	return 0;
} // }}}
static ssize_t       data_default_is_null       (data_t *data, fastcall_is_null *fargs){ // {{{
	fargs->is_null = (data->ptr == NULL) ? 1 : 0;
	return 0;
} // }}}
static ssize_t       data_default_init          (data_t *dst, fastcall_init *fargs){ // {{{
	data_t                 d_initstr         = DATA_STRING(fargs->string);
	
	fastcall_convert_from r_convert = { { 3, ACTION_CONVERT_FROM }, &d_initstr };
	return data_query(dst, &r_convert);
} // }}}
//static ssize_t       data_default_convert       (data_t *src, fastcall_convert *fargs){ // {{{
//	ssize_t                ret;
//	
//	fastcall_convert_to   r_convert_to   = { { fargs->header.nargs, ACTION_CONVERT_TO   }, fargs->dest, fargs->format };
//	if( (ret = data_query(src, &r_convert_to)) != -ENOSYS )
//		return ret;
//	
//	fastcall_convert_from r_convert_from = { { fargs->header.nargs, ACTION_CONVERT_FROM }, src,         fargs->format };
//	return data_query(fargs->dest, &r_convert_from);
//} // }}}

data_proto_t default_t_proto = {
	.type            = TYPE_DEFAULTT,
	.type_str        = "",
	.api_type        = API_HANDLERS,
	.handlers        = {
		[ACTION_COPY]        = (f_data_func)&data_default_copy,
		[ACTION_COMPARE]     = (f_data_func)&data_default_compare,
		[ACTION_READ]        = (f_data_func)&data_default_read,
		[ACTION_WRITE]       = (f_data_func)&data_default_write,
		[ACTION_TRANSFER]    = (f_data_func)&data_default_transfer,
		[ACTION_FREE]        = (f_data_func)&data_default_free,
		[ACTION_GETDATAPTR]  = (f_data_func)&data_default_getdataptr,
		[ACTION_IS_NULL]     = (f_data_func)&data_default_is_null,
		[ACTION_INIT]        = (f_data_func)&data_default_init,
	}
};

