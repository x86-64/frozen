#include <libfrozen.h>

size_t  data_void_raw_len(void *data, size_t buffer_size){
	return buffer_size;
}

REGISTER_DATA(TYPE_VOID,SIZE_VARIABLE, .func_len = &data_void_raw_len)

