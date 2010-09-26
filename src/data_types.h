typedef enum data_type {
	TYPE_INT8,
	TYPE_INT16,
	TYPE_INT32,
	TYPE_INT64,
} data_type;

int int8_cmp_func(void *data1, void *data2);
int int16_cmp_func(void *data1, void *data2);
int int32_cmp_func(void *data1, void *data2);
int int64_cmp_func(void *data1, void *data2);

