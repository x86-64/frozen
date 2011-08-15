#include <libfrozen.h>
#include <buffer_t.h>
#include <uint/off_t.h>
#include <uint/size_t.h>

ssize_t   data_buffer_t_read  (data_t *data, data_ctx_t *context, off_t offset, void **buffer, size_t *buffer_size){
	ssize_t   ret;
	off_t     d_offset = 0;
	size_t    d_size   = 0;
	void     *chunk;
	
	hash_data_copy(ret, TYPE_OFFT,  d_offset, context, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, d_size,   context, HK(size)); (void)ret;
	
	if(d_size != 0 && offset >= d_size)
		return -1; // EOF
		
	if(buffer_seek((buffer_t *)data->data_ptr, d_offset + offset, &chunk, buffer, buffer_size) != 0)
		return -1; // EOF
	
	*buffer_size = (d_size == 0) ? *buffer_size : MIN(d_size, *buffer_size);
	
	return *buffer_size;
}

ssize_t   data_buffer_t_write (data_t *data, data_ctx_t *context, off_t offset, void *buffer, size_t size){
	ssize_t  ret;
	off_t    d_offset = 0;
	
	hash_data_copy(ret, TYPE_OFFT,  d_offset, context, HK(offset)); (void)ret;
	
	buffer_write((buffer_t *)data->data_ptr, d_offset + offset, buffer, size);
	
	return size;
}

data_proto_t buffer_t_proto = {
	.type                   = TYPE_BUFFERT,
	.type_str               = "buffer_t",
	.size_type              = SIZE_VARIABLE,
	.func_read              = &data_buffer_t_read,
	.func_write             = &data_buffer_t_write
};
/* vim: set filetype=c: */
