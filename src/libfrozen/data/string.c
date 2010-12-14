#include <libfrozen.h>

ssize_t   data_string_read  (data_t *data, data_ctx_t *context, off_t offset, void **buffer, size_t *buffer_size){
	off_t     d_offset;
	size_t    d_size, str_l;
	char     *str;
	hash_t   *temp;
	
	// read limits
	d_offset =
		( (temp = hash_find_typed(context, TYPE_OFFT, "offset")) != NULL) ?
		HVALUE(temp, off_t) : 0;
	d_size =
		( (temp = hash_find_typed(context, TYPE_SIZET, "size")) != NULL) ?
		HVALUE(temp, size_t) : data->data_size;
	
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
	off_t    d_offset;
	size_t   d_size;
	hash_t  *temp;
	
	d_offset =
		( (temp = hash_find_typed(context, TYPE_OFFT, "offset")) != NULL) ?
		HVALUE(temp, off_t) : 0;
	d_size =
		( (temp = hash_find_typed(context, TYPE_SIZET, "size")) != NULL) ?
		HVALUE(temp, size_t) : data->data_size;
	
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

int     data_string_cmp(data_t *data1, data_ctx_t *ctx1, data_t *data2, data_ctx_t *ctx2){
	// TODO IMPORTANT use ctx
	return strcmp(data1->data_ptr, data2->data_ptr);
}

size_t  data_string_len(data_t *data, data_ctx_t *ctx){
	// TODO IMPORTANT use ctx
	return (size_t) MIN( strlen(data->data_ptr) + 1, data->data_size );
}

ssize_t data_string_convert(data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){ // {{{
	size_t size;
	
	switch(src->type){
		case TYPE_STRING:
			size = data_len(src, src_ctx) + 1;
			
			if( (dst->data_ptr = malloc(size)) == NULL)
				return -ENOMEM;
			
			dst->type      = TYPE_STRING;
			dst->data_size = size;
			
			if(data_read(src, src_ctx, 0, dst->data_ptr, size) < 0)
				return -EINVAL;
			
			*((char *)dst->data_ptr + size - 1) = '\0';
			return 0;
		
		default:
			break;
	};
	return -ENOSYS;
} // }}}


/*
REGISTER_TYPE(`TYPE_STRING')
REGISTER_MACRO(`DATA_STRING(value)',      `TYPE_STRING, value, sizeof(value)')
REGISTER_MACRO(`DATA_PTR_STRING(value,size)', `TYPE_STRING, value, size')
REGISTER_PROTO(
	`{
		.type          = TYPE_STRING,
		.type_str      = "string",
		.size_type     = SIZE_VARIABLE,
		.func_cmp      = &data_string_cmp,
		.func_len      = &data_string_len,
		.func_read     = &data_string_read,
		.func_write    = &data_string_write,
		.func_convert  = &data_string_convert
	}')
*/
