#include <libfrozen.h>

ssize_t   data_string_read  (data_t *data, data_ctx_t *context, off_t offset, void **buffer, size_t *buffer_size){
	ssize_t   ret;
	off_t     d_offset = 0;
	size_t    d_size   = data->data_size, str_l;
	char     *str;
	
	// read limits
	hash_data_copy(ret, TYPE_OFFT,  d_offset, context, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, d_size,   context, HK(size));
	
	// check consistency
	if(d_offset > data->data_size || d_size > data->data_size || d_offset + d_size > data->data_size)
		return -EINVAL;  // invalid context parameters
	
	if(offset > d_size)
		return -EINVAL;  // invalid request
	
	d_offset += offset;
	d_size   -= offset;
	d_size    = MIN(*buffer_size, d_size);
	
	// find string length
	str   = data->data_ptr + d_offset;
	str_l = 0;
	while(str_l < d_size && *str++ != 0){
		str_l++;
	}
	
	if(str_l == 0)
		return -1;       // EOF
	
	*buffer      = data->data_ptr + d_offset;
	*buffer_size = str_l + 1;
	
	return str_l;
}

ssize_t   data_string_write (data_t *data, data_ctx_t *context, off_t offset, void *buffer, size_t size){
	ssize_t  ret;
	off_t    d_offset = 0;
	size_t   d_size   = data->data_size;
	
	hash_data_copy(ret, TYPE_OFFT,  d_offset, context, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, d_size,   context, HK(size));
	
	if(d_offset > data->data_size || d_size > data->data_size || d_offset + d_size > data->data_size)
		return -EINVAL;  // invalid context parameters
	
	if(offset > d_size)
		return -EINVAL;  // invalid request
	
	d_offset += offset;
	d_size   -= offset;
	
	d_size    = MIN(size, d_size);
	
	if(d_size == 0)
		return -EINVAL;  // bad request
	
	size_t written = 0;
	char c, *ptr = data->data_ptr + d_offset;
	do{
		c = *ptr++ = *(char *)buffer++;
		written++;
	}while(c);
	return written;
}

// TODO possible crash here
// TODO new api

int     data_string_cmp(data_t *data1, data_ctx_t *ctx1, data_t *data2, data_ctx_t *ctx2){
	(void)ctx1; (void)ctx2; // TODO IMPORTANT use ctx
	return strcmp(data1->data_ptr, data2->data_ptr);
}

size_t  data_string_len(data_t *data, data_ctx_t *ctx){
	(void)ctx; // TODO IMPORTANT use ctx
	return (size_t) MIN( strlen(data->data_ptr) + 1, data->data_size );
}

size_t data_string_len2raw(size_t unitsize){ // {{{
	return unitsize + 1;
} // }}}


ssize_t data_string_convert(data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){ // {{{
	ssize_t size;
	char    nullchar = '\0';
	
	switch(src->type){
		case TYPE_STRING:
			if( (size = data_transfer(dst, dst_ctx, src, src_ctx)) < 0)
				return -EFAULT;
			
			data_write(dst, dst_ctx, size, &nullchar, sizeof(nullchar));
			return 0;
		
		default:
			break;
	};
	return -ENOSYS;
} // }}}

/*
REGISTER_TYPE(`TYPE_STRING')
REGISTER_MACRO(`DATA_STRING(value)',          `TYPE_STRING, value, sizeof(value)')
REGISTER_MACRO(`DATA_PTR_STRING(value,size)', `TYPE_STRING, value, size')
REGISTER_MACRO(`DATA_PTR_STRING_AUTO(value)', `TYPE_STRING, value, strlen(value)+1')
REGISTER_RAW_MACRO(`DT_STRING', `char *')
REGISTER_RAW_MACRO(`GET_TYPE_STRING(value)', `(char *)value->data_ptr')
REGISTER_PROTO(
	`{
		.type          = TYPE_STRING,
		.type_str      = "string",
		.size_type     = SIZE_VARIABLE,
		.func_cmp      = &data_string_cmp,
		.func_len      = &data_string_len,
		.func_read     = &data_string_read,
		.func_write    = &data_string_write,
		.func_convert  = &data_string_convert,
		.func_len2raw  = &data_string_len2raw
	}')
*/
