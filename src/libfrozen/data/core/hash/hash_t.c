#include <libfrozen.h>
#include <hash_t.h>

#include <enum/format/format_t.h>
#include <enum/datatype/datatype_t.h>
#include <core/string/string_t.h>
#include <storage/raw/raw_t.h>

#include <modifiers/slider/slider_t.h>

#define HASH_INITIAL_SIZE 8

typedef struct hash_t_ctx {
	hash_t                *hash;
	uintmax_t              step;
	uintmax_t              buffer_data_offset;
	data_t                *sl_holder;
	data_t                *sl_data;
} hash_t_ctx;

static void data_hash_t_append_pad(hash_t_ctx *ctx){ // {{{
	fastcall_write r_write = { { 5, ACTION_WRITE }, 0, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t", ctx->step };
	data_query(ctx->sl_data, &r_write);
} // }}}
static void data_hash_t_append(hash_t_ctx *ctx, char *string){ // {{{
	fastcall_write r_write = { { 5, ACTION_WRITE }, 0, string, strlen(string) };
	data_query(ctx->sl_data, &r_write);
} // }}}

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
	ssize_t                ret;
	data_t                 d_key             = DATA_PTR_HASHKEYT(&hash_item->key);
	data_t                 d_type            = DATA_PTR_DATATYPET(&hash_item->data.type);
	
	if(hash_item->key == 0) // skip deleted keys
		return ITER_CONTINUE;
	
	if(hash_item->data.type == TYPE_MACHINET) // HACK HACK HACK
		return ITER_CONTINUE;
	
	if(hash_item->key != hash_ptr_end && hash_item != hash_find(ctx->hash, hash_item->key)) // skip duplicates
		return ITER_CONTINUE;
	
	switch(ctx->step){
		case 0:; // step one: count all elements
			fastcall_length r_length = { { 4, ACTION_LENGTH }, 0, FORMAT(packed) };
			
			if( (ret = data_query(&d_key,  &r_length)) < 0)
				return ret;
			ctx->buffer_data_offset += r_length.length;
			
			if( (ret = data_query(&d_type, &r_length)) < 0)
				return ret;
			ctx->buffer_data_offset += r_length.length;
			
			break;

		case 1:; // step two: write to buffer
			fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_TO }, ctx->sl_holder, FORMAT(packed) };
			
			// write header
			if( (ret = data_query(&d_key,  &r_convert)) < 0)
				return ret;
			if( (ret = data_query(&d_type, &r_convert)) < 0)
				return ret;
			
			// write data
			data_slider_t_freeze(ctx->sl_data);
				
				fastcall_convert_to r_convert2 = { { 4, ACTION_CONVERT_TO }, ctx->sl_data, FORMAT(packed) };
				ret = data_query(&hash_item->data, &r_convert2);
			
			data_slider_t_unfreeze(ctx->sl_data);
			return (ret < 0) ? ITER_BREAK : ITER_CONTINUE;
	};
	return ITER_CONTINUE;
} // }}}
static ssize_t data_hash_t_convert_from_iter(hash_t *hash_item, hash_t_ctx *ctx){ // {{{
	ssize_t                ret;
	
	data_slider_t_freeze(ctx->sl_data);
	
		fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, ctx->sl_data, FORMAT(packed) };
		ret = data_query(&hash_item->data, &r_convert);
	
	data_slider_t_unfreeze(ctx->sl_data);
	
	return (ret < 0) ? ITER_BREAK : ITER_CONTINUE;
} // }}}
static ssize_t data_hash_t_convert_to_debug_iter(hash_t *element, hash_t_ctx *ctx){ // {{{
	unsigned int           k;
	char                   buffer[DEF_BUFFER_SIZE];
	char                   b_s_key[DEF_BUFFER_SIZE];
	char                   b_s_type[DEF_BUFFER_SIZE];
	data_t                 d_s_key           = DATA_RAW(b_s_key,  sizeof(b_s_key));
	data_t                 d_s_type          = DATA_RAW(b_s_type, sizeof(b_s_type));
	
	if(element->key == hash_ptr_null){
		data_hash_t_append_pad(ctx);
		data_hash_t_append(ctx, " - hash_null\n");
		return ITER_CONTINUE;
	}
	if(element->key == hash_ptr_end){
		data_hash_t_append_pad(ctx);
		data_hash_t_append(ctx, " - hash_end\n");
		
		if(ctx->step > 0)
			ctx->step -= 1;
		return ITER_CONTINUE;
	}
	if(element->key == hash_ptr_inline){
		data_hash_t_append_pad(ctx);
		data_hash_t_append(ctx, " - hash_inline:\n");
		
		ctx->step += 1;
		return ITER_CONTINUE;
	}

	data_t              d_key     = DATA_HASHKEYT(element->key);
	fastcall_convert_to r_convert1 = { { 5, ACTION_CONVERT_TO }, &d_s_key,  FORMAT(human) };
	data_query(&d_key, &r_convert1);
	
	data_t              d_type    = DATA_DATATYPET(element->data.type);
	fastcall_convert_to r_convert2 = { { 5, ACTION_CONVERT_TO }, &d_s_type, FORMAT(human) };
	data_query(&d_type, &r_convert2);
	
	snprintf(buffer, sizeof(buffer),
		" - %.*s [%.*s] -> %p",
		(int)r_convert1.transfered, (char *)b_s_key,
		(int)r_convert2.transfered, (char *)b_s_type,
		element->data.ptr
	);
	data_hash_t_append_pad(ctx);
	data_hash_t_append(ctx, buffer);
	
	fastcall_getdataptr r_ptr = { { 3, ACTION_GETDATAPTR } };
	fastcall_length     r_len = { { 4, ACTION_LENGTH }, 0, FORMAT(native) };
	if(data_query(&element->data, &r_ptr) == 0 && data_query(&element->data, &r_len) == 0){
		for(k = 0; k < r_len.length; k++){
			if((k % 32) == 0){
				snprintf(buffer, sizeof(buffer), "   0x%.5x: ", k);
				
				data_hash_t_append(ctx, "\n");
				data_hash_t_append_pad(ctx);
				data_hash_t_append(ctx, buffer);
			}
			
			snprintf(buffer, sizeof(buffer), "%.2hhx ", (unsigned int)(*((char *)r_ptr.ptr + k)));
			data_hash_t_append(ctx, buffer);
		}
	}
	data_hash_t_append(ctx, "\n");
	
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
	hash_t_ctx             ctx               = { .hash = src->ptr };
	
	switch(fargs->format){
		case FORMAT(native):
		case FORMAT(packed):
			if(hash_iter(ctx.hash, (hash_iterator)&data_hash_t_convert_to_iter, &ctx, HASH_ITER_NULL | HASH_ITER_END) != ITER_OK)
				return -EFAULT;
			
			data_t                 d_sl_holder       = DATA_SLIDERT(fargs->dest, 0);
			data_t                 d_sl_data         = DATA_SLIDERT(fargs->dest, ctx.buffer_data_offset);
			
			ctx.step++;
			ctx.sl_holder = &d_sl_holder;
			ctx.sl_data   = &d_sl_data;
			
			if(hash_iter(ctx.hash, (hash_iterator)&data_hash_t_convert_to_iter, &ctx, HASH_ITER_NULL | HASH_ITER_END) != ITER_OK)
				return -EFAULT;
			
			break;
			
		case FORMAT(debug):;
			data_t                 d_sl_data2        = DATA_SLIDERT(fargs->dest, 0);
			
			ctx.step      = 0;
			ctx.sl_data   = &d_sl_data2;
			
			if(hash_iter((hash_t *)src->ptr, (hash_iterator)&data_hash_t_convert_to_debug_iter, &ctx, HASH_ITER_NULL | HASH_ITER_INLINE | HASH_ITER_END) != ITER_OK)
				return -EFAULT;
			break;
			
		default:
			return -ENOSYS;
	}
	return 0;
} // }}}
static ssize_t data_hash_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	hash_t                *hash;
	hash_t                *curr;
	data_t                *old_data;
	uintmax_t              nelements         = 0;
	uintmax_t              hash_nelements    = 0;
	data_t                 sl_src            = DATA_SLIDERT(fargs->src, 0);
	list                   freeit            = LIST_INITIALIZER;
	
	switch(fargs->format){
		case FORMAT(native):
		case FORMAT(packed): break;
		case FORMAT(human):
		case FORMAT(config):
		case FORMAT(hash):
			dst->ptr = NULL;
			return 0;
		default:
			return -ENOSYS;
	}
	
	hash_nelements = HASH_INITIAL_SIZE;
	hash           = hash_new(hash_nelements);
	
	for(curr = hash;; curr++){
		data_t       d_key          = DATA_PTR_HASHKEYT(&curr->key);
		data_t       d_type         = DATA_PTR_DATATYPET(&curr->data.type);

		fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, &sl_src, FORMAT(packed) };
		
		if( (ret = data_query(&d_key, &r_convert)) < 0)
			return ret;
		if( (ret = data_query(&d_type, &r_convert)) < 0)
			return ret;
		
		if(curr->key == hash_ptr_end)
			break;
		
		// redata data holders if any
		if(dst->ptr != NULL){
			if( (old_data = hash_data_find((hash_t *)dst->ptr, curr->key)) != NULL){
				curr->data.ptr  = old_data->ptr;
			}else{
				curr->data.ptr  = NULL;
				list_add(&freeit, &curr->data);
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
	}
	
	// redata data
	hash_t_ctx ctx = { .sl_data = &sl_src };
	ret = (hash_iter(hash, (hash_iterator)&data_hash_t_convert_from_iter, &ctx, 0) == ITER_OK) ?
		0 :
		-EFAULT;
	
	if(dst->ptr == NULL){
		dst->ptr = (ret == 0) ? hash : NULL; // return hash if status ok
	}else{
		fastcall_free r_free = { { 3, ACTION_FREE } };
		while( (old_data = list_pop(&freeit)) != NULL)
			data_query(old_data, &r_free);
		
		free(hash);      // no need for this hash - we redata data in old one
	}
	return ret;
} // }}}

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

hash_t *           hash_new                     (size_t nelements){ // {{{
	size_t  i;
	hash_t *hash;
	
	if(__MAX(size_t) / sizeof(hash_t) <= nelements)
		return NULL;
	
	if( (hash = malloc(nelements * sizeof(hash_t))) == NULL)
		return NULL;
	
	for(i=0; i<nelements - 1; i++){
		hash_assign_hash_null(&hash[i]);
	}
	hash_assign_hash_end(&hash[nelements-1]);
	
	return hash;
} // }}}
hash_t *           hash_copy                    (hash_t *hash){ // {{{
	unsigned int  k;
	hash_t       *el, *el_new, *new_hash;
	
	if(hash == NULL)
		return NULL;
	
	for(el = hash, k=0; el->key != hash_ptr_end; el++){
		if(el->key == hash_ptr_null)
			continue;
		
		k++;
	}
	if( (new_hash = malloc((k+1)*sizeof(hash_t))) == NULL )
		return NULL;
	
	for(el = hash, el_new = new_hash; el->key != hash_ptr_end; el++){
		if(el->key == hash_ptr_null)
			continue;

		el_new->key       = el->key;
		el_new->data.type = el->data.type;
		el_new->data.ptr  = NULL;
		
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &el_new->data };
		data_query(&el->data, &r_copy);
		
		el_new++;
	}
	hash_assign_hash_end(el_new);
	
	return new_hash;
} // }}}
void               hash_free                    (hash_t *hash){ // {{{
	hash_t       *el;
	
	if(hash == NULL)
		return;
	
	for(el = hash; el->key != hash_ptr_end; el++){
		if(el->key == hash_ptr_null)
			continue;
		
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(&el->data, &r_free);
	}
	
	free(hash);
} // }}}

hash_t *           hash_find                    (hash_t *hash, hashkey_t key){ // {{{
	register hash_t       *inline_hash;
	
	for(; hash != NULL; hash = (hash_t *)hash->data.ptr){
		goto loop_start;
		do{
			hash++;

		loop_start:
			if(hash->key == key)
				return hash;
			
			if(hash->key == hash_ptr_inline){
				if( (inline_hash = hash_find((hash_t *)hash->data.ptr, key)) != NULL)
					return inline_hash;
			}
		}while(hash->key != hash_ptr_end);
	}
	return NULL;
} // }}}
ssize_t            hash_iter                    (hash_t *hash, hash_iterator func, void *arg, hash_iter_flags flags){ // {{{
	ssize_t                ret;
	hash_t                *curr              = hash;
	
	if(hash == NULL)
		return ITER_BREAK;
	
	do{
		// null items
		if( curr->key == hash_ptr_null && ( (flags & HASH_ITER_NULL) == 0 ) )
			goto next;
		
		// inline items
		if( curr->key == hash_ptr_inline ){
			if(curr->data.ptr == NULL) // empty inline item
				goto next;
			
			if( (flags & HASH_ITER_INLINE) != 0 ){
				if( (ret = func(curr, arg)) != ITER_CONTINUE)
					return ret;
			}

			if( (ret = hash_iter(
				(hash_t *)curr->data.ptr,
				func,
				arg,
				flags & ~HASH_ITER_END            // don't show hash_end in inline hashes 
			)) != ITER_OK)
				return ret;
			
			goto next;
		}
		
		if(curr->key == hash_ptr_end)
			break;

		// regular items
		if( (ret = func(curr, arg)) != ITER_CONTINUE)
			return ret;
		
	next:	
		curr++;
	}while(1);
	
	if((flags & HASH_ITER_END) != 0){
		if( (ret = func(curr, arg)) != ITER_CONTINUE)
			return ret;
	}
	
	return ITER_OK;
} // }}}
size_t             hash_nelements               (hash_t *hash){ // {{{
	hash_t       *el;
	unsigned int  i;
	
	if(hash == NULL)
		return 0;
	
	for(el = hash, i=0; el->key != hash_ptr_end; el++){
		i++;
	}
	return i + 1;
} // }}}

data_t *           hash_data_find               (hash_t *hash, hashkey_t key){ // {{{
	hash_t *temp;
	return ((temp = hash_find(hash, key)) == NULL) ?
		NULL : &temp->data;
} // }}}

void               hash_dump                    (hash_t *hash){ // {{{
	char                   buffer[DEF_BUFFER_SIZE*20] = { 0 };
	data_t                 d_hash            = DATA_PTR_HASHT(hash);
	data_t                 d_buffer          = DATA_RAW(buffer, sizeof(buffer));
	
	fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_TO }, &d_buffer, FORMAT(debug) };
	if(data_query(&d_hash, &r_convert) < 0)
		return;
	
	fprintf(stderr, "%s", buffer);
} // }}}

