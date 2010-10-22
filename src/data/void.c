#include <libfrozen.h>

size_t  data_void_bare_len(void *data, size_t buffer_size){
	return buffer_size;
}

/*
REGISTER_TYPE(`TYPE_VOID')
REGISTER_MACRO(`DATA_VOID', `TYPE_VOID, NULL, 0')
REGISTER_PROTO(
	`{
		.type          = TYPE_VOID,
		.type_str      = "void",
		.size_type     = SIZE_VARIABLE,
		.func_bare_len = &data_void_bare_len
	}')
*/
