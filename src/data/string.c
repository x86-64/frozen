#include <libfrozen.h>

int     data_string_cmp(void *data1, void *data2){
	return strcmp(data1, data2);
}

// TODO possible crash here
size_t  data_string_len(void *data, size_t buffer_size){
	return (size_t) MIN( strlen(data) + 1, buffer_size );
}

/*
REGISTER_TYPE(`TYPE_STRING')
REGISTER_MACRO(`DATA_STRING(value)',      `TYPE_STRING, value, sizeof(value)')
REGISTER_MACRO(`DATA_PTR_STRING(value,size)', `TYPE_STRING, value, size')
REGISTER_PROTO(
	`{
		.type          = TYPE_STRING,
		.type_str      = "string",
		.size_type     = SIZE_VARIABLE,
		.func_bare_cmp = &data_string_cmp,
		.func_bare_len = &data_string_len
	}')
*/
