#include <libfrozen.h>

size_t  data_void_len(data_t *data, data_ctx_t *ctx){
	return data->data_size;
}

/*
REGISTER_TYPE(`TYPE_VOID')
REGISTER_MACRO(`DATA_VOID', `TYPE_VOID, NULL, 0')
REGISTER_PROTO(
	`{
		.type          = TYPE_VOID,
		.type_str      = "void",
		.size_type     = SIZE_VARIABLE,
		.func_len      = &data_void_len
	}')
*/
