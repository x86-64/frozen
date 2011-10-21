#include <libfrozen.h>
#include <struct_t.h>
#include <slice/slice_t.h>

static ssize_t   data_struct_t_read  (data_t *data, fastcall_read *fargs){
	return -1;
}
static ssize_t   data_struct_t_write (data_t *data, fastcall_write *fargs){
	return -1;
}

typedef struct struct_iter_ctx {
	request_t             *values;
	off_t                  curr_offset;
	data_t                *buffer;
	uintmax_t              buffer_size;
} struct_iter_ctx;

static ssize_t  struct_iter_pack(hash_t *element, void *p_ctx){
	data_t                *curr_data;
	data_t                 need_data;
	uintmax_t              need_data_free    = 0;
	struct_iter_ctx       *iter_ctx          = (struct_iter_ctx *)p_ctx;
	
	// find value for current key
	if( (curr_data = hash_data_find(iter_ctx->values, element->key)) == NULL)
		return ITER_BREAK; // alloc empty value and free it after transferring
	
	// convert if need
	if(curr_data->type != element->data.type){
		need_data.type = element->data.type;
		need_data.ptr  = NULL;
		
		fastcall_convert r_convert = { { 3, ACTION_CONVERT }, &need_data }; 
		if(data_query(curr_data, &r_convert) != 0)
			return ITER_BREAK;
		
		curr_data      = &need_data;
		need_data_free = 1;
	}
	
	// get length ( + backroom check )
	fastcall_physicallen r_len = { { 3, ACTION_PHYSICALLEN } };
	if(data_query(curr_data, &r_len) != 0)
		return ITER_BREAK;
	
	// make new slice from buffer
	data_t d_slice = DATA_SLICET(iter_ctx->buffer, iter_ctx->curr_offset, ~0);
	
	// write data to buffer
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_slice };
	if(data_query(curr_data, &r_transfer) != 0)
		return ITER_BREAK;
	
	// update offset
	iter_ctx->curr_offset += r_len.length;
	
	if(need_data_free == 1){
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(curr_data, &r_free);
	}
	return ITER_CONTINUE;
}

static ssize_t  struct_iter_unpack(hash_t *element, void *p_ctx){
	data_t                *curr_data;
	data_t                 need_data;
	uintmax_t              need_data_free          = 0;
	struct_iter_ctx       *iter_ctx                = (struct_iter_ctx *)p_ctx;
	
	if(iter_ctx->curr_offset >= iter_ctx->buffer_size)
		return ITER_BREAK;
	
	// make new slice from buffer
	data_t d_slice = DATA_SLICET(iter_ctx->buffer, iter_ctx->curr_offset, ~0);
	
	// prepare data
	if( (curr_data = hash_data_find(iter_ctx->values, element->key)) == NULL){
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &need_data };          // default values
		if(data_query(&element->data, &r_copy) != 0)
			return ITER_BREAK;
		
		curr_data      = &need_data;
		need_data_free = 1;
	}
	// TODO convert user-supplied buffer to struct type. For dataptr_t it will work, but not for uint_t ans simmilar
	
	// read data from buffer
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, curr_data };
	if(data_query(&d_slice, &r_transfer) != 0)
		return ITER_BREAK;
	
	// get length
	fastcall_physicallen r_len = { { 3, ACTION_PHYSICALLEN } };
	if(data_query(curr_data, &r_len) != 0)
		return ITER_BREAK;
	
	iter_ctx->curr_offset += r_len.length;
	
	if(need_data_free == 1){
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(curr_data, &r_free);
	}

	return ITER_CONTINUE;
}

/** Pack hash with values, using structure into buffer */
size_t    struct_pack      (struct_t *structure, request_t *values, data_t *buffer){
	struct_iter_ctx  iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = buffer;
	iter_ctx.curr_offset = 0;
	
	if(hash_iter(structure, &struct_iter_pack, &iter_ctx, 0) == ITER_BREAK)
		return 0;
	
	return (size_t)iter_ctx.curr_offset;
}

/** Unpack buffer to values using structure */
size_t    struct_unpack    (struct_t *structure, request_t *values, data_t *buffer){
	struct_iter_ctx  iter_ctx;
	
	fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
	if(data_query(buffer, &r_len) != 0)
		return -EINVAL;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = buffer;
	iter_ctx.buffer_size = r_len.length;
	iter_ctx.curr_offset = 0;
	
	if(hash_iter(structure, &struct_iter_unpack, &iter_ctx, 0) == ITER_BREAK)
		return 0;
	
	return (size_t)iter_ctx.curr_offset;
}
	
data_proto_t struct_t_proto = {
	.type                   = TYPE_STRUCTT,
	.type_str               = "struct_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_READ]   = (f_data_func)&data_struct_t_read,
		[ACTION_WRITE]  = (f_data_func)&data_struct_t_write
	}
};

