#include <libfrozen.h>

data_t data_types[] = {
	{ "int8",TYPE_INT8,SIZE_FIXED,&int8_cmp_func,1 },
	{ "int16",TYPE_INT16,SIZE_FIXED,&int16_cmp_func,2 },
	{ "int32",TYPE_INT32,SIZE_FIXED,&int32_cmp_func,4 },
	{ "int64",TYPE_INT64,SIZE_FIXED,&int64_cmp_func,8 },
};
size_t data_types_size = sizeof(data_types) / sizeof(data_t);

