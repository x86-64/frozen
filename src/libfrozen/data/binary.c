#include <libfrozen.h>

typedef struct data_binary_t {
	unsigned int size;
	// data here
} data_binary_t;

/*
int     data_binary_buff_cmp(data_ctx_t *ctx, buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off){
	int           cret;
	unsigned int  item1_len = 0;
	unsigned int  item2_len = 0;	
	
	buffer_read(buffer1, buffer1_off, &item1_len, sizeof(item1_len));
	buffer_read(buffer2, buffer2_off, &item2_len, sizeof(item2_len));
	     if(item1_len > item2_len){ cret =  1; }
	else if(item1_len < item2_len){ cret = -1; }
	else {
		cret = buffer_memcmp(
			buffer1, buffer1_off + sizeof(item1_len),
			buffer2, buffer2_off + sizeof(item2_len),
			item1_len);
		if(cret > 0) cret =  1;
		if(cret < 0) cret = -1;
	}
	return cret;
}*/
void *  data_binary_ptr(data_t *data){
	return (void *)((data_binary_t *)data + 1);
}

size_t  data_binary_len(data_t *data, data_ctx_t *ctx){
	unsigned int data_size;
	
	if(data->data_size < sizeof(data_binary_t))
		return 0;
	
	data_size = ((data_binary_t *)(data->data_ptr))->size;
	if(data_size > data->data_size)
		return 0;
	
	return (size_t)data_size;
}


int     data_binary_cmp(data_t *data1, data_ctx_t *ctx1, data_t *data2, data_ctx_t *ctx2){
	int           cret;
	size_t        item1_len, item2_len;	
	
	item1_len = data_binary_len(data1, ctx1);
	item2_len = data_binary_len(data2, ctx2);
	     if(item1_len > item2_len){ cret =  1; }
	else if(item1_len < item2_len){ cret = -1; }
	else {
		cret = memcmp(data_binary_ptr(data1), data_binary_ptr(data2), item1_len);
		if(cret > 0) cret =  1;
		if(cret < 0) cret = -1;
	}
	return cret;
}

/*
REGISTER_TYPE(`TYPE_BINARY')
REGISTER_PROTO(
	`{
		.type            = TYPE_BINARY,
		.type_str        = "binary",
		.size_type       = SIZE_VARIABLE,
		.func_cmp        = &data_binary_cmp,
		.func_len        = &data_binary_len,
	}')
*/
