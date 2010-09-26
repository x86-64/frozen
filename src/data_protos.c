#include <libfrozen.h>

data_proto_t data_protos[] = {
	{ "binary",TYPE_BINARY,SIZE_VARIABLE,&data_binary_cmp,&data_binary_len },
	{ "int8",TYPE_INT8,SIZE_FIXED,&int8_cmp_func,NULL,1 },
	{ "int16",TYPE_INT16,SIZE_FIXED,&int16_cmp_func,NULL,2 },
	{ "int32",TYPE_INT32,SIZE_FIXED,&int32_cmp_func,NULL,4 },
	{ "int64",TYPE_INT64,SIZE_FIXED,&int64_cmp_func,NULL,8 },
};
size_t data_protos_size = sizeof(data_protos) / sizeof(data_proto_t);

