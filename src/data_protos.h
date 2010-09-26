#ifndef DATA_PROTOS_H
#define DATA_PROTOS_H
typedef enum data_type {
	TYPE_BINARY,
	TYPE_INT8,
	TYPE_INT16,
	TYPE_INT32,
	TYPE_INT64,
} data_type;

int     data_binary_cmp(void *data1, void *data2);
size_t  data_binary_len(void *data);
void *  data_binary_ptr(void *data);
int int8_cmp_func(void *data1, void *data2);
int int16_cmp_func(void *data1, void *data2);
int int32_cmp_func(void *data1, void *data2);
int int64_cmp_func(void *data1, void *data2);


#endif // DATA_PROTOS_H