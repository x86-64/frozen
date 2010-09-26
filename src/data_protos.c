#include <libfrozen.h>

data_proto_t data_protos[] = {
	{ "binary",TYPE_BINARY,SIZE_VARIABLE, .func_cmp = &data_binary_cmp, .func_len = &data_binary_raw_len },
	{ "int16",TYPE_INT16,SIZE_FIXED, .func_cmp = &data_int16_cmp, .func_inc = &data_int16_inc, .func_div = &data_int16_div, .func_mul = &data_int16_mul, .fixed_size = 2 },
	{ "int32",TYPE_INT32,SIZE_FIXED, .func_cmp = &data_int32_cmp, .func_inc = &data_int32_inc, .func_div = &data_int32_div, .func_mul = &data_int32_mul, .fixed_size = 4 },
	{ "int64",TYPE_INT64,SIZE_FIXED, .func_cmp = &data_int64_cmp, .func_inc = &data_int64_inc, .func_div = &data_int64_div, .func_mul = &data_int64_mul, .fixed_size = 8 },
	{ "int8",TYPE_INT8,SIZE_FIXED, .func_cmp = &data_int8_cmp, .func_inc = &data_int8_inc, .func_div = &data_int8_div, .func_mul = &data_int8_mul, .fixed_size = 1 },
};
size_t data_protos_size = sizeof(data_protos) / sizeof(data_proto_t);

