#include <libfrozen.h>
#include <dataproto.h>
#include <hash_t.h>

#include <raw/raw_t.h>
#include <slider/slider_t.h>

#define HASH_INITIAL_SIZE 8

typedef struct hash_t_ctx {
	uintmax_t              step;
	uintmax_t              buffer_data_offset;
	data_t                *sl_holder;
	data_t                *sl_data;
} hash_t_ctx;

static ssize_t data_hash_t_compare_iter(hash_t *hash1_item, fastcall_compare *fargs){ // {{{
	hash_t                *hash2_item;
	
	if( (hash2_item = hash_find(fargs->data2->ptr, hash1_item->key)) == NULL)
		goto not_equal;
	
	fastcall_compare r_compare = { { 4, ACTION_COMPARE }, &hash2_item->data };
	if(data_query(&hash1_item->data, &r_compare) != 0)
		goto not_equal;
	
	if(r_compare.result != 0){
		fargs->result = r_compare.result;
		return ITER_BREAK;
	}
	
	return ITER_CONTINUE;

not_equal:
	fargs->result = 2;
	return ITER_BREAK;
} // }}}
static ssize_t data_hash_t_convert_to_iter(hash_t *hash_item, hash_t_ctx *ctx){ // {{{
	switch(ctx->step){
		case 0:; // step one: count all elements
			ctx->buffer_data_offset += sizeof(hash_t);
			break;
		case 1:; // step two: write to buffer
			
			// write holder
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, hash_item, sizeof(hash_t) };
			if(data_query(ctx->sl_holder, &r_write) < 0)
				return ITER_BREAK;
			
			if(hash_item->key != hash_ptr_end && hash_item->key != hash_ptr_null){
				// write data
				fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_TO }, ctx->sl_data, FORMAT_BINARY };
				if(data_query(&hash_item->data, &r_convert) < 0)
					return ITER_BREAK;
			}
			break;
	};
	return ITER_CONTINUE;
} // }}}
static ssize_t data_hash_t_convert_from_iter(hash_t *hash_item, hash_t_ctx *ctx){ // {{{
	fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, ctx->sl_data, FORMAT_BINARY };
	if(data_query(&hash_item->data, &r_convert) < 0)
		return ITER_BREAK;
	
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
	hash_t                *hash1;
	
	if(fargs->data2 == NULL || fargs->data2->type != TYPE_HASHT)
		return -EINVAL;
	
	hash1 = data1->ptr;
	
	// null and empty hash equals to any hash
	if(hash1 == NULL || hash_iter(hash1, (hash_iterator)&data_hash_t_compare_iter, fargs, 0) == ITER_OK)
		fargs->result = 0;
	
	return 0;
} // }}}

static ssize_t data_hash_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	hash_t_ctx             ctx               = {};
	
	if(hash_iter((hash_t *)src->ptr, (hash_iterator)&data_hash_t_convert_to_iter, &ctx, HASH_ITER_NULL | HASH_ITER_END) != ITER_OK)
		return -EFAULT;
	
	data_t                 d_sl_holder       = DATA_SLIDERT(fargs->dest, 0);
	data_t                 d_sl_data         = DATA_SLIDERT(fargs->dest, ctx.buffer_data_offset);
	
	ctx.step++;
	ctx.sl_holder = &d_sl_holder;
	ctx.sl_data   = &d_sl_data;
	
	if(hash_iter((hash_t *)src->ptr, (hash_iterator)&data_hash_t_convert_to_iter, &ctx, HASH_ITER_NULL | HASH_ITER_END) != ITER_OK)
		return -EFAULT;
	
	return 0;
} // }}}
static ssize_t data_hash_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	hash_t                *hash;
	hash_t                *curr;
	data_t                *old_data;
	uintmax_t              nelements         = 0;
	uintmax_t              hash_nelements    = 0;
	data_t                 sl_src            = DATA_SLIDERT(fargs->src, 0);
	list                   new_data          = LIST_INITIALIZER;
	
	if(fargs->format != FORMAT_BINARY)
		return -ENOSYS;
	
	hash_nelements = HASH_INITIAL_SIZE;
	hash           = hash_new(hash_nelements);
	
	for(curr = hash;; curr++){ 
		fastcall_read r_read = {
			{ 5, ACTION_READ },
			0,
			curr,
			sizeof(hash_t)
		};
		if(data_query(&sl_src, &r_read) < 0)
			return -EFAULT;
		
		// restore data holders if any
		if(dst->ptr != NULL){
			if( (old_data = hash_data_find((hash_t *)dst->ptr, curr->key)) != NULL){
				curr->data.ptr  = old_data->ptr;
			}else{
				curr->data.ptr  = NULL;
				list_add(&new_data, &curr->data);
			}
		}else{
			curr->data.ptr  = NULL;
		}

		nelements++;
		if(nelements + 1 == hash_nelements){
			// expand hash
			hash_t             *new_hash;
			
			hash_nelements *= 2;
			new_hash = hash_new(hash_nelements);
			memcpy(new_hash, hash, nelements * sizeof(hash_t));
			free(hash); // !!! free, not hash_free
			
			hash = new_hash;
			curr = new_hash + (curr - hash);
		}

		if(curr->key == hash_ptr_end)
			break;
	}
	
	// restore data
	hash_t_ctx ctx = { .sl_data = &sl_src };
	if(hash_iter(hash, (hash_iterator)&data_hash_t_convert_from_iter, &ctx, 0) != ITER_OK){
		hash_free(hash);
		return -EFAULT;
	}
	
	if(dst->ptr == NULL){
		dst->ptr = hash; // return hash
	}else{
		fastcall_free r_free = { { 3, ACTION_FREE } };
		while( (old_data = list_pop(&new_data)) != NULL)
			data_query(old_data, &r_free);
		
		free(hash);      // no need for this hash - we restore data in old one
	}
	return 0;
} // }}}

/*
static ssize_t data_hash_t_read(data_t *data, fastcall_read *fargs){ // {{{
	if(fargs->offset != 0) // TODO force read to respect offset
		return -EINVAL;
	
	data_t d_buffer = DATA_RAW(fargs->buffer, fargs->buffer_size);
	
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_buffer };
	return data_hash_t_transfer(data, &r_transfer);
} // }}}
*/

data_proto_t hash_t_proto = {
	.type                   = TYPE_HASHT,
	.type_str               = "hash_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_COPY]         = (f_data_func)&data_hash_t_copy,
		[ACTION_FREE]         = (f_data_func)&data_hash_t_free,
		[ACTION_COMPARE]      = (f_data_func)&data_hash_t_compare,
		[ACTION_CONVERT_TO]   = (f_data_func)&data_hash_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_hash_t_convert_from,
	}
};
