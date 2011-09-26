#include <libfrozen.h>
#include <struct_t.h>
#include <uint/off_t.h>

static ssize_t   data_struct_t_read  (data_t *data, fastcall_read *fargs){
	return -1;
}
static ssize_t   data_struct_t_write (data_t *data, fastcall_write *fargs){
	return -1;
}

typedef struct struct_iter_ctx {
	request_t  *values;
	data_t     *buffer;
	off_t       curr_offset;
} struct_iter_ctx;

static ssize_t  struct_iter_pack(hash_t *element, void *p_ctx, void *null){
	//ssize_t          ret;
	data_t          *curr_data;
	//data_t           need_data;
	struct_iter_ctx *iter_ctx = (struct_iter_ctx *)p_ctx;
	
	if( (curr_data = hash_data_find(iter_ctx->values, element->key)) == NULL)
		return ITER_BREAK;
	
	//DT_OFFT    new_offset = 0;
	//hash_data_copy(ret, TYPE_OFFT, new_offset, iter_ctx->ctx, HK(offset));
	//new_offset += iter_ctx->curr_offset;
	//data_ctx_t new_ctx[] = {
	//	{ HK(offset), DATA_PTR_OFFT(&new_offset) },
	//	hash_next(iter_ctx->ctx)
	//};
	//if( (ret = data_transfer(iter_ctx->buffer, new_ctx, curr_data)) < 0)
	//	return ITER_BREAK;
	
	// TODO IMPORTANT bad packing, TYPE_BINARYT in troubles
	
	////data_convert_to_local(ret, data_value_type(hash_item_data(element)), &need_data, curr_data);
	//size_t m_len = data_len(_src,_src_ctx);                  
	//m_len = data_len2raw(_type, m_len);                      
	//data_alloc_local(_dst,_type,m_len);                      
	//_retval = data_convert(_dst,NULL,_src,_src_ctx);         
	//if(ret < 0)
	//	return ITER_BREAK;
	
	//size_t rawlen = data_rawlen(&need_data, NULL);
	
	//ret = data_write(
	//	iter_ctx->buffer, iter_ctx->ctx, iter_ctx->curr_offset,
	//	data_value_ptr(&need_data), rawlen
	//);
	
	//iter_ctx->curr_offset += ret;
	
	return ITER_CONTINUE;
}

static ssize_t  struct_iter_unpack(hash_t *element, void *p_ctx, void *null){
	data_t          *curr_data, new_data;
	struct_iter_ctx *iter_ctx = (struct_iter_ctx *)p_ctx;
	
	//if(iter_ctx->curr_offset >= iter_ctx->buffer->data_size)
	//	return ITER_BREAK;
	
	new_data.type      = element->data.type;
	new_data.ptr  = iter_ctx->buffer->ptr + iter_ctx->curr_offset;
	//new_data.data_size = iter_ctx->buffer->data_size - iter_ctx->curr_offset;
	
	//new_data.data_size = data_rawlen(&new_data, NULL);
	//iter_ctx->curr_offset += new_data.data_size;
	
	if( (curr_data = hash_data_find(iter_ctx->values, element->key)) != NULL){
		memcpy(curr_data, &new_data, sizeof(new_data));
	}
	
	return ITER_CONTINUE;
}

static ssize_t  struct_iter_unpack_copy(hash_t *element, void *p_ctx, void *null){
	data_t          *curr_data, new_data;
	struct_iter_ctx *iter_ctx = (struct_iter_ctx *)p_ctx;
	
	//if(iter_ctx->curr_offset >= iter_ctx->buffer->data_size)
	//	return ITER_BREAK;
	
	new_data.type      = element->data.type;
	new_data.ptr  = iter_ctx->buffer->ptr + iter_ctx->curr_offset;
	//new_data.data_size = iter_ctx->buffer->data_size - iter_ctx->curr_offset;
	
	//new_data.data_size = data_rawlen(&new_data, NULL);
	//iter_ctx->curr_offset += new_data.data_size;
	
	if( (curr_data = hash_data_find(iter_ctx->values, element->key)) != NULL){
		(void)new_data;
//		data_transfer(curr_data, &new_data);
	}
	
	return ITER_CONTINUE;
}

/** Pack hash with values, using structure into buffer */
size_t    struct_pack      (struct_t *structure, request_t *values, data_t *buffer){
	struct_iter_ctx  iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = buffer;
	//iter_ctx.ctx         = ctx;
	iter_ctx.curr_offset = 0;
	if(hash_iter(structure, &struct_iter_pack, &iter_ctx, NULL) == ITER_BREAK)
		return 0;
	
	return (size_t)iter_ctx.curr_offset;
}

/** Unpack buffer to values using structure */
size_t    struct_unpack    (struct_t *structure, request_t *values, data_t *buffer){
	//ssize_t          ret;
	struct_iter_ctx  iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = buffer;
	iter_ctx.curr_offset = 0;
	
	//hash_data_copy(ret, TYPE_OFFT, iter_ctx.curr_offset, HK(offset)); (void)ret;
	//if(iter_ctx.curr_offset >= buffer->data_size)
	//	return 0;
	// TODO size
	
	if(hash_iter(structure, &struct_iter_unpack, &iter_ctx, NULL) == ITER_BREAK)
		return 0;
	
	return (size_t)iter_ctx.curr_offset;
}

/** Unpack buffer to values using structure. Copy data instead of pointing to buffer */
size_t    struct_unpack_copy  (struct_t *structure, request_t *values, data_t *buffer){
	//ssize_t          ret;
	struct_iter_ctx  iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = buffer;
	iter_ctx.curr_offset = 0;
	
	//hash_data_copy(ret, TYPE_OFFT, iter_ctx.curr_offset, ctx, HK(offset)); (void)ret;
	//if(iter_ctx.curr_offset >= buffer->data_size)
	//	return 0;
	// TODO size
	
	if(hash_iter(structure, &struct_iter_unpack_copy, &iter_ctx, NULL) == ITER_BREAK)
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

