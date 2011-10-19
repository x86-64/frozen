#include <libfrozen.h>
#include <hash_t.h>
#include <raw/raw_t.h>
#include <slider/slider_t.h>

typedef struct data_hash_t_items_iter {
	fastcall_transfer     *fargs;
	uintmax_t              step;
	uintmax_t              buffer_data_offset;
	data_t                *sl_holder;
	data_t                *sl_data;
} data_hash_t_items_iter;

static ssize_t data_hash_t_compare_iter(hash_t *hash1_item, hash_t *hash2, void *null){ // {{{
	hash_t                *hash2_item;

	if( (hash2_item = hash_find(hash2, hash1_item->key)) == NULL)
		return ITER_BREAK;
	
	fastcall_compare r_compare = { { 4, ACTION_COMPARE }, &hash2_item->data };
	if(data_query(&hash1_item->data, &r_compare) != 0)
		return ITER_BREAK;
	
	return ITER_CONTINUE;
} // }}}
static ssize_t data_hash_t_transfer_iter(hash_t *hash_item, data_hash_t_items_iter *ctx, void *null){ // {{{
	switch(ctx->step){
		case 0:; // step one: count all elements
			ctx->buffer_data_offset += sizeof(hash_t);
			break;
		case 1:; // step two: write to buffer
			
			// write holder
			data_t            d_holder    = DATA_RAW(hash_item, sizeof(hash_t));
			fastcall_transfer r_transfer1 = { { 3, ACTION_TRANSFER }, ctx->sl_holder };
			if(data_query(&d_holder, &r_transfer1) < 0)
				return ITER_BREAK;
			
			// write data
			fastcall_transfer r_transfer2 = { { 3, ACTION_TRANSFER }, ctx->sl_data };
			if(data_query(&hash_item->data, &r_transfer2) < 0)
				return ITER_BREAK;
			
			break;
	};
	return ITER_CONTINUE;
} // }}}

static ssize_t data_hash_t_copy(data_t *src, fastcall_copy *fargs){ // {{{
	hash_t                *src_hash          = (hash_t *)src->ptr;
	data_t                *dst_data          = (data_t *)fargs->dest;
	
	if( (dst_data->ptr = hash_copy(src_hash)) == NULL)
		return -EFAULT;
	
	dst_data->type = src->type;
	return 0;
} // }}}
static ssize_t data_hash_t_free(data_t *data, fastcall_free *fargs){ // {{{
	hash_free(data->ptr);
	return 0;
} // }}}
static ssize_t data_hash_t_compare(data_t *data1, fastcall_compare *fargs){ // {{{
	hash_t                *hash1, *hash2;
	
	if(fargs->data2 == NULL || fargs->data2->type != TYPE_HASHT)
		return -EINVAL;
	
	hash1 = data1->ptr;
	hash2 = fargs->data2->ptr;

	if(hash_iter(hash1, (hash_iterator)&data_hash_t_compare_iter, hash2, NULL) == ITER_OK){
		return 0;
	}
	return 2;
} // }}}
static ssize_t data_hash_t_transfer(data_t *data, fastcall_transfer *fargs){ // {{{
	data_hash_t_items_iter ctx               = { fargs };
	
	if(hash_iter((hash_t *)data->ptr, (hash_iterator)&data_hash_t_transfer_iter, &ctx, NULL) != ITER_OK)
		return -EFAULT;
	
	data_t                 d_sl_holder       = DATA_SLIDERT(fargs->dest, 0);
	data_t                 d_sl_data         = DATA_SLIDERT(fargs->dest, ctx.buffer_data_offset);
	
	ctx.step++;
	ctx.sl_holder = &d_sl_holder;
	ctx.sl_data   = &d_sl_data;
	
	if(hash_iter((hash_t *)data->ptr, (hash_iterator)&data_hash_t_transfer_iter, &ctx, NULL) != ITER_OK)
		return -EFAULT;
	
	return 0;
} // }}}
static ssize_t data_hash_t_read(data_t *data, fastcall_read *fargs){ // {{{
	if(fargs->offset != 0) // TODO force read to respect offset
		return -EINVAL;
	
	data_t d_buffer = DATA_RAW(fargs->buffer, fargs->buffer_size);
	
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_buffer };
	return data_hash_t_transfer(data, &r_transfer);
} // }}}

/*
// macros {{{
#define _hash_to_memory_cpy(_ptr,_size) { \
	if(_size > memory_size) \
		return -ENOMEM; \
	memcpy(memory, _ptr, _size); \
	memory      += _size; \
	memory_size -= _size; \
}
#define _hash_from_memory_cpy(_ptr,_size) { \
	if(_size > memory_size) \
		return -ENOMEM; \
	memcpy(_ptr, memory, _size); \
	memory      += _size; \
	memory_size -= _size; \
}
// }}}
static ssize_t hash_to_buffer_one(hash_t *hash, void *p_buffer, void *p_null){ // {{{
	void     *data_ptr;
	
	data_ptr  = hash->data.ptr;
	
	if(data_ptr == NULL)
		return ITER_CONTINUE;
	
	fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
	if(data_query(&hash->data, &r_len) < 0)
		return ITER_BREAK;
	
	buffer_add_tail_raw((buffer_t *)p_buffer, data_ptr, r_len.length);
	return ITER_CONTINUE;
} // }}}
ssize_t            hash_to_buffer               (hash_t  *hash, buffer_t *buffer){ // {{{
	size_t  nelements;
	
	buffer_init(buffer);
	
	nelements = hash_nelements(hash);
	buffer_add_tail_raw(buffer, hash, nelements * sizeof(hash_t));
	
	if(hash_iter(hash, &hash_to_buffer_one, buffer, NULL) == ITER_BREAK)
		goto error;
	
	return 0;
error:
	buffer_free(buffer);
	return -EFAULT;
} // }}}
ssize_t            hash_from_buffer             (hash_t **hash, buffer_t *buffer){ // {{{
	hash_t    *curr;
	void      *memory, *chunk, *ptr;
	size_t     size, data_size;
	off_t      data_off = 0;
	
	if(buffer_seek(buffer, 0, &chunk, &memory, &size) < 0)
		goto error;
	
	for(curr = (hash_t *)memory; curr->key != hash_ptr_end; curr++){
		if(size < sizeof(hash_t))
			goto error;
		
		size        -= sizeof(hash_t);
		data_off    += sizeof(hash_t);
	}
	data_off += sizeof(hash_t);
	
	for(curr = (hash_t *)memory; curr->key != hash_ptr_end; curr++){
		if(curr->key == hash_ptr_null)
			continue;
		
		if(buffer_seek(buffer, data_off, &chunk, &ptr, &size) < 0)
			goto error;
		
		data_size = curr->data.len;
		
		if(data_size > size)
			goto error;
		
		curr->data->type = curr->data.ptr;
		curr->data->ptr  = ptr;
		
		data_off += data_size;
	}	
	
	*hash = (hash_t *)memory;
	return 0;
error:
	return -EFAULT;
} // }}}
ssize_t            hash_to_memory               (hash_t  *hash, void *memory, size_t memory_size){ // {{{
	size_t  i, nelements, hash_size;
	
	nelements = hash_nelements(hash);
	hash_size = nelements * sizeof(hash_t);
	
	_hash_to_memory_cpy(hash, hash_size);
	for(i=0; i<nelements; i++, hash++){
		if(hash->data.ptr == NULL || hash->data.data_size == 0)
			continue;
		
		_hash_to_memory_cpy(hash->data.ptr, hash->data.data_size);
	}
	return 0;
} // }}}
ssize_t            hash_reread_from_memory      (hash_t  *hash, void *memory, size_t memory_size){ // {{{
	size_t  i, nelements, hash_size;
	
	nelements = hash_nelements(hash);
	hash_size = nelements * sizeof(hash_t);
	
	memory      += hash_size;
	memory_size -= hash_size;
	for(i=0; i<nelements; i++, hash++){
		if(hash->data.ptr == NULL || hash->data.data_size == 0)
			continue;
		if(hash->key == hash_ptr_null)
			continue;
		
		_hash_from_memory_cpy(hash->data.ptr, hash->data.data_size);
	}
	return 0;
} // }}}
ssize_t            hash_from_memory             (hash_t **hash, void *memory, size_t memory_size){ // {{{
	hash_t    *curr;
	size_t     data_size;
	off_t      data_off = 0;
	
	for(curr = (hash_t *)memory; curr->key != hash_ptr_end; curr++){
		if(memory_size < sizeof(hash_t))
			goto error;
		
		memory_size -= sizeof(hash_t);
		data_off    += sizeof(hash_t);
	}
	data_off += sizeof(hash_t);
	
	for(curr = (hash_t *)memory; curr->key != hash_ptr_end; curr++){
		if(curr->key == hash_ptr_null)
			continue;
		
		data_size = curr->data.data_size;
		
		if(data_size > memory_size)
			goto error;
		
		curr->data.ptr = memory + data_off;
		//data_assign_raw(
		//	&curr->data,
		//	curr->data.type,
		//	memory + data_off,
		//	data_size
		//);
		memory_size -= data_size;
		data_off    += data_size;
	}	
	
	*hash = (hash_t *)memory;
	return 0;
error:
	return -EFAULT;
} // }}}
*/

data_proto_t hash_t_proto = {
	.type                   = TYPE_HASHT,
	.type_str               = "hash_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_COPY]     = (f_data_func)&data_hash_t_copy,
		[ACTION_FREE]     = (f_data_func)&data_hash_t_free,
		[ACTION_COMPARE]  = (f_data_func)&data_hash_t_compare,
		[ACTION_TRANSFER] = (f_data_func)&data_hash_t_transfer,
		[ACTION_READ]     = (f_data_func)&data_hash_t_read,
	}
};
