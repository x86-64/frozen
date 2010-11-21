#include <libfrozen.h>

ssize_t   data_buffer_t_read  (data_t *data, data_ctx_t *context, off_t offset, void **buffer, size_t *buffer_size){
	off_t     d_offset;
	size_t    d_size;
	hash_t   *temp;
	void     *chunk;
	
	d_offset =
		( (temp = hash_find_typed(context, TYPE_OFFT, "offset")) != NULL) ?
		HVALUE(temp, off_t) : 0;
	d_size =
		( (temp = hash_find_typed(context, TYPE_SIZET, "size")) != NULL) ?
		HVALUE(temp, size_t) : 0;
		
	if(d_size != 0 && offset >= d_size)
		return -1; // EOF
		
	if(buffer_seek((buffer_t *)data->data_ptr, d_offset + offset, &chunk, buffer, buffer_size) != 0)
		return -1; // EOF
	
	*buffer_size = (d_size == 0) ? *buffer_size : MIN(d_size, *buffer_size);
	
	return *buffer_size;
}

ssize_t   data_buffer_t_write (data_t *data, data_ctx_t *context, off_t offset, void *buffer, size_t size){
	off_t    d_offset;
	hash_t  *temp;
	
	d_offset =
		( (temp = hash_find_typed(context, TYPE_OFFT, "offset")) != NULL) ?
		HVALUE(temp, off_t) : 0;
	
	buffer_write((buffer_t *)data->data_ptr, d_offset + offset, buffer, size);
	
	return size;
}

/*
REGISTER_TYPE(TYPE_BUFFERT)
REGISTER_MACRO(`DATA_BUFFERT(_buff)', `TYPE_BUFFERT, (void *)_buff, 0')
REGISTER_PROTO(
	`{
		.type                   = TYPE_BUFFERT,
		.type_str               = "buffer_t",
		.size_type              = SIZE_VARIABLE,
		.func_read              = &data_buffer_t_read,
		.func_write             = &data_buffer_t_write
	}'
)
*/
/* vim: set filetype=c: */
