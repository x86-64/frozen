#include <libfrozen.h>

ssize_t   data_io_t_read  (data_t *data, data_ctx_t *ctx, off_t offset, void **buffer, size_t *buffer_size){
	return ((io_t *)data->data_ptr)->read(data, ctx, offset, buffer, buffer_size);
}

ssize_t   data_io_t_write (data_t *data, data_ctx_t *ctx, off_t offset, void *buffer, size_t size){
	return ((io_t *)data->data_ptr)->write(data, ctx, offset, buffer, size);
}

/*
REGISTER_TYPE(TYPE_IOT)
REGISTER_MACRO(`DATA_IOT(_fread, _fwrite)', `TYPE_IOT, (io_t []){ { _fread, _fwrite } }, 0')
REGISTER_STRUCT(`
	typedef struct io_t {
		f_data_read      read;
		f_data_write     write;
	} io_t;
')
REGISTER_PROTO(
	`{
		.type                   = TYPE_IOT,
		.type_str               = "io_t",
		.size_type              = SIZE_VARIABLE,
		.func_read              = &data_io_t_read,
		.func_write             = &data_io_t_write
	}'
)
*/

