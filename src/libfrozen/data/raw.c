#include <libfrozen.h>

size_t  data_raw_len(data_t *data, data_ctx_t *ctx){
	(void)ctx; // TODO add ctx parse
	return data->data_size;
}

/*
REGISTER_TYPE(`TYPE_RAW')
REGISTER_MACRO(`DATA_RAW(_buf,_size)',        `TYPE_RAW, _buf, _size')
REGISTER_PROTO(
	`{
		.type          = TYPE_RAW,
		.type_str      = "raw",
		.size_type     = SIZE_VARIABLE,
		.func_len      = &data_raw_len,
	}')
*/

