#include <libfrozen.h>

size_t  data_void_len(data_t *data, data_ctx_t *ctx){
	return data->data_size;
}

ssize_t data_void_convert(data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){ // {{{
	dst->type      = TYPE_VOID;
	dst->data_ptr  = NULL;
	dst->data_size = 0;
	return 0;
} // }}}

/*
REGISTER_TYPE(`TYPE_VOID')
REGISTER_MACRO(`DATA_VOID', `TYPE_VOID, NULL, 0')
REGISTER_PROTO(
	`{
		.type          = TYPE_VOID,
		.type_str      = "void",
		.size_type     = SIZE_VARIABLE,
		.func_len      = &data_void_len,
		.func_convert  = &data_void_convert
	}')
*/
