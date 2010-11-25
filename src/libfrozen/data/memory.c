#include <libfrozen.h>

size_t  data_memory_len(data_t *data, data_ctx_t *ctx){
	return data->data_size;
}

/*
REGISTER_TYPE(`TYPE_MEMORY')
REGISTER_MACRO(`DATA_MEM(_buf,_size)',        `TYPE_MEMORY, _buf, _size')
REGISTER_PROTO(
	`{
		.type          = TYPE_MEMORY,
		.type_str      = "memory",
		.size_type     = SIZE_VARIABLE,
		.func_len      = &data_memory_len,
	}')
*/

