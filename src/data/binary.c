#include <libfrozen.h>

typedef struct data_binary_t {
	unsigned int size;
	// data here
} data_binary_t;

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
}

size_t  data_binary_buff_len(data_ctx_t *ctx, buffer_t *buffer, off_t buffer_off){
	unsigned int   item_len = 0;
	
	buffer_read(buffer, buffer_off, &item_len, sizeof(item_len));
	
	return (size_t) ( MIN(item_len, buffer_get_size(buffer) - sizeof(item_len)) );
}

int     data_binary_bare_cmp(void *data1, void *data2){
	int           cret;
	unsigned int  item1_len, item2_len;	
	
	item1_len = ((data_binary_t *)data1)->size;
	item2_len = ((data_binary_t *)data2)->size;
	     if(item1_len > item2_len){ cret =  1; }
	else if(item1_len < item2_len){ cret = -1; }
	else {
		cret = memcmp((data_binary_t *)data1 + 1, (data_binary_t *)data2 + 1, item1_len);
		if(cret > 0) cret =  1;
		if(cret < 0) cret = -1;
	}
	return cret;
}

size_t  data_binary_bare_len(void *data, size_t buffer_size){
	size_t data_size;
	
	data_size = (size_t)(((data_binary_t *)data)->size);
	if(data_size < sizeof(data_binary_t) || data_size > buffer_size)
		return 0;
	
	return data_size;
}

size_t  data_binary_len(void *data){
	size_t data_size;
	
	data_size = (size_t)(((data_binary_t *)data)->size);
	
	return (data_size - sizeof(data_binary_t));
}

void *  data_binary_ptr(void *data){
	return (void *)((data_binary_t *)data + 1);
}

/*
REGISTER_TYPE(`TYPE_BINARY')
REGISTER_PROTO(
	`{
		.type            = TYPE_BINARY,
		.type_str        = "binary",
		.size_type       = SIZE_VARIABLE,
		.func_bare_cmp   = &data_binary_bare_cmp,
		.func_bare_len   = &data_binary_bare_len,
		.func_buffer_cmp = &data_binary_buff_cmp,
		.func_buffer_len = &data_binary_buff_len
	}')
*/
