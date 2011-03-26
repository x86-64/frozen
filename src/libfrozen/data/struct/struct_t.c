#include <libfrozen.h>
#include <struct_t.h>
#include <uint/off_t.h>

ssize_t   data_struct_t_read  (data_t *data, data_ctx_t *ctx, off_t offset, void **buffer, size_t *buffer_size){
	return -1;
}
ssize_t   data_struct_t_write (data_t *data, data_ctx_t *ctx, off_t offset, void *buffer, size_t size){
	return -1;
}

typedef struct struct_iter_ctx {
	request_t  *values;
	data_t     *buffer;
	data_ctx_t *ctx;
	off_t       curr_offset;
} struct_iter_ctx;

static ssize_t  struct_iter_pack(hash_t *element, void *p_ctx, void *null){
	ssize_t          ret;
	data_t          *curr_data;
	data_ctx_t      *curr_ctx;
	data_t           need_data;
	struct_iter_ctx *iter_ctx = (struct_iter_ctx *)p_ctx;
	
	hash_data_find(iter_ctx->values, element->key, &curr_data, &curr_ctx);
	
	if(curr_data == NULL){
		data_proto_t *proto = data_proto_from_type(element->data.type);
		if(proto == NULL)
			return ITER_BREAK;
		
		switch(proto->size_type){
			case SIZE_FIXED:
				iter_ctx->curr_offset += proto->fixed_size;
				break;
			case SIZE_VARIABLE:
				// not supported currently
				return ITER_BREAK;
		};
	}else{
		//DT_OFFT    new_offset = 0;
		//hash_data_copy(ret, TYPE_OFFT, new_offset, iter_ctx->ctx, HK(offset));
		//new_offset += iter_ctx->curr_offset;
		//data_ctx_t new_ctx[] = {
		//	{ HK(offset), DATA_PTR_OFFT(&new_offset) },
		//	hash_next(iter_ctx->ctx)
		//};
		//if( (ret = data_transfer(iter_ctx->buffer, new_ctx, curr_data, curr_ctx)) < 0)
		//	return ITER_BREAK;
		
		// TODO IMPORTANT bad packing, TYPE_BINARY in troubles
		
		data_convert_to_local(ret, data_value_type(hash_item_data(element)), &need_data, curr_data, curr_ctx);
		if(ret < 0)
			return ITER_BREAK;
		
		size_t rawlen = data_rawlen(&need_data, NULL);
		
		ret = data_write(
			iter_ctx->buffer, iter_ctx->ctx, iter_ctx->curr_offset,
			data_value_ptr(&need_data), rawlen
		);
		
		iter_ctx->curr_offset += ret;
	}
	
	return ITER_CONTINUE;
}

static ssize_t  struct_iter_unpack(hash_t *element, void *p_ctx, void *null){
	data_t          *curr_data, new_data;
	struct_iter_ctx *iter_ctx = (struct_iter_ctx *)p_ctx;
	
	if(iter_ctx->curr_offset >= iter_ctx->buffer->data_size)
		return ITER_BREAK;
	
	new_data.type      = element->data.type;
	new_data.data_ptr  = iter_ctx->buffer->data_ptr + iter_ctx->curr_offset;
	new_data.data_size = iter_ctx->buffer->data_size - iter_ctx->curr_offset;
	
	new_data.data_size = data_rawlen(&new_data, NULL);
	iter_ctx->curr_offset += new_data.data_size;
	
	hash_data_find(iter_ctx->values, element->key, &curr_data, NULL);
	if(curr_data != NULL){
		memcpy(curr_data, &new_data, sizeof(new_data));
	}
	
	return ITER_CONTINUE;
}
/** Pack hash with values, using structure into buffer */
size_t    struct_pack      (struct_t *structure, request_t *values, data_t *buffer, data_ctx_t *ctx){
	struct_iter_ctx  iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = buffer;
	iter_ctx.ctx         = ctx;
	iter_ctx.curr_offset = 0;
	if(hash_iter(structure, &struct_iter_pack, &iter_ctx, NULL) == ITER_BREAK)
		return 0;
	
	return (size_t)iter_ctx.curr_offset;
}

/** Unpack buffer to values using structure */
size_t    struct_unpack    (struct_t *structure, request_t *values, data_t *buffer, data_ctx_t *ctx){
	ssize_t          ret;
	struct_iter_ctx  iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = buffer;
	iter_ctx.ctx         = ctx;
	iter_ctx.curr_offset = 0;
	
	hash_data_copy(ret, TYPE_OFFT, iter_ctx.curr_offset, ctx, HK(offset));
	if(iter_ctx.curr_offset >= buffer->data_size)
		return 0;
	// TODO size
	
	if(hash_iter(structure, &struct_iter_unpack, &iter_ctx, NULL) == ITER_BREAK)
		return 0;
	
	return (size_t)iter_ctx.curr_offset;
}

data_proto_t struct_proto = {
		.type                   = TYPE_STRUCTT,
		.type_str               = "struct_t",
		.size_type              = SIZE_VARIABLE,
		.func_read              = &data_struct_t_read,
		.func_write             = &data_struct_t_write
};

