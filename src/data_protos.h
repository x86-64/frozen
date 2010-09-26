#ifndef DATA_PROTOS_H
#define DATA_PROTOS_H
typedef enum data_type {
	TYPE_BINARY,
	TYPE_INT16,
	TYPE_INT32,
	TYPE_INT64,
	TYPE_INT8,
	TYPE_VOID,
} data_type;

int     data_binary_cmp(void *data1, void *data2);
size_t  data_binary_raw_len(void *data, size_t buffer_size);
size_t  data_binary_len(void *data);
void *  data_binary_ptr(void *data);
int data_int16_cmp(void *data1, void *data2);
int data_int16_inc(void *data);
int data_int16_div(void *data, unsigned int divider);
int data_int16_mul(void *data, unsigned int mul);
int data_int32_cmp(void *data1, void *data2);
int data_int32_inc(void *data);
int data_int32_div(void *data, unsigned int divider);
int data_int32_mul(void *data, unsigned int mul);
int data_int64_cmp(void *data1, void *data2);
int data_int64_inc(void *data);
int data_int64_div(void *data, unsigned int divider);
int data_int64_mul(void *data, unsigned int mul);
int data_int8_cmp(void *data1, void *data2);
int data_int8_inc(void *data);
int data_int8_div(void *data, unsigned int divider);
int data_int8_mul(void *data, unsigned int mul);
size_t  data_void_raw_len(void *data, size_t buffer_size);


#endif // DATA_PROTOS_H