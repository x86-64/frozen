#include <libfrozen.h>
#include <io_t.h>

ssize_t   data_io_t_read  (data_t *data, data_ctx_t *ctx, off_t offset, void **buffer, size_t *buffer_size){
	return ((io_t *)data->data_ptr)->read(data, ctx, offset, buffer, buffer_size);
}

ssize_t   data_io_t_write (data_t *data, data_ctx_t *ctx, off_t offset, void *buffer, size_t size){
	return ((io_t *)data->data_ptr)->write(data, ctx, offset, buffer, size);
}

data_proto_t io_proto = {
	.type                   = TYPE_IOT,
	.type_str               = "io_t",
	.size_type              = SIZE_VARIABLE,
	.func_read              = &data_io_t_read,
	.func_write             = &data_io_t_write
};
