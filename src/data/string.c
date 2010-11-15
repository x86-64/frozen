#include <libfrozen.h>

// TODO possible crash here

int     data_string_cmp(data_t *data1, data_ctx_t *ctx1, data_t *data2, data_ctx_t *ctx2){
	return strcmp(data1->data_ptr, data2->data_ptr);
}

size_t  data_string_len(data_t *data, data_ctx_t *ctx){
	return (size_t) MIN( strlen(data->data_ptr) + 1, data->data_size );
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
		.func_cmp      = &data_string_cmp,
		.func_len      = &data_string_len
	}')
*/
