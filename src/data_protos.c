#include <libfrozen.h>

data_proto_t data_protos[] = {
	{ "binary",TYPE_BINARY,SIZE_VARIABLE, .func_cmp = &data_binary_cmp, .func_len = &data_binary_len },
	{ "int8",TYPE_INT8,SIZE_FIXED, .func_cmp = &data_int8_cmp, .func_inc = &data_int8_inc, .fixed_size = 1 },
	{ "int16",TYPE_INT16,SIZE_FIXED, .func_cmp = &data_int16_cmp, .func_inc = &data_int16_inc, .fixed_size = 2 },
	{ "int32",TYPE_INT32,SIZE_FIXED, .func_cmp = &data_int32_cmp, .func_inc = &data_int32_inc, .fixed_size = 4 },
	{ "int64",TYPE_INT64,SIZE_FIXED, .func_cmp = &data_int64_cmp, .func_inc = &data_int64_inc, .fixed_size = 8 },
};
size_t data_protos_size = sizeof(data_protos) / sizeof(data_proto_t);

