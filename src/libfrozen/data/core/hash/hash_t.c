#include <libfrozen.h>
#include <hash_t.h>

#include <enum/format/format_t.h>
#include <enum/datatype/datatype_t.h>
#include <storage/raw/raw_t.h>
#include <io/fd/fd_t.h>
#include <modifiers/slider/slider_t.h>
#include <core/data/data_t.h>
#include <special/consumable/consumable_t.h>
#include <numeric/uint/uint_t.h>

#define HASH_INITIAL_SIZE 8

typedef struct hash_t_ctx {
	hash_t                *hash;
	uintmax_t              step;
	data_t                *sl_data;
	ssize_t                ret;
} hash_t_ctx;

typedef struct hash_t_rebuild_ctx {
	uintmax_t              nelements;
	hash_t                *hash;
} hash_t_rebuild_ctx;

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
	data_t                 d_data            = DATA_PTR_DATAT(&hash_item->data);
	
	if(hash_item->key == 0) // skip deleted keys
		return ITER_CONTINUE;
	
	if(hash_item->key != hash_ptr_end && hash_item != hash_find(ctx->hash, hash_item->key)) // skip duplicates
		return ITER_CONTINUE;
	
	fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, ctx->sl_data, FORMAT(packed) };
	
	// write header
	if( (ret = data_query(&d_key,  &r_convert)) < 0)
		return ITER_BREAK;
	
	data_slider_t_set_offset(ctx->sl_data, r_convert.transfered, SEEK_CUR);

	// write data
	if( (ret = data_query(&d_data, &r_convert)) < 0)
		return ITER_BREAK;
	
	data_slider_t_set_offset(ctx->sl_data, r_convert.transfered, SEEK_CUR);
	
	return ITER_CONTINUE;
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
	
	fastcall_view r_view = { { 6, ACTION_VIEW }, FORMAT(native) };
	if(data_query(&element->data, &r_view) == 0){
		for(k = 0; k < r_view.length; k++){
			if((k % 32) == 0){
				snprintf(buffer, sizeof(buffer), "   0x%.5x: ", k);
				
				data_hash_t_append(ctx, "\n");
				data_hash_t_append_pad(ctx);
				data_hash_t_append(ctx, buffer);
			}
			
			snprintf(buffer, sizeof(buffer), "%.2hhx ", (unsigned int)(*((char *)r_view.ptr + k)));
			data_hash_t_append(ctx, buffer);
		}
	}
	data_free(&r_view.freeit);
	data_hash_t_append(ctx, "\n");
	
	return ITER_CONTINUE;
} // }}}
static ssize_t data_hash_t_enum_iter(hash_t *hash_item, hash_t_ctx *ctx){ // {{{
	data_t                 d_key             = DATA_PTR_HASHKEYT(&hash_item->key);
	data_t                 n_key             = DATA_NOTCONSUMABLET(d_key);
	data_t                 n_value           = DATA_NOTCONSUMABLET(hash_item->data);
	
	fastcall_create        r_create          = { { 4, ACTION_CREATE }, &n_key, &n_value };
	ctx->ret = data_query(ctx->sl_data, &r_create);
	
	hash_item->data = ((consumable_t *)(n_value.ptr))->data; // HACK, restore value (can change during convert)
	
	return (ctx->ret < 0) ? ITER_BREAK : ITER_CONTINUE;
} // }}}

static ssize_t data_hash_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr)
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
		case FORMAT(packed):;
			data_t                 d_sl_data         = DATA_SLIDERT(fargs->dest, 0);
			
			ctx.sl_data   = &d_sl_data;
			
			if(hash_iter(ctx.hash, (hash_iterator)&data_hash_t_convert_to_iter, &ctx, HASH_ITER_END) != ITER_OK)
				return -EFAULT;
			
			if(fargs->header.nargs >= 5)
				fargs->transfered = data_slider_t_get_offset(&d_sl_data);
			
			break;
			
		case FORMAT(debug):;
			data_t                 d_sl_data2        = DATA_AUTO_SLIDERT(fargs->dest, 0);
			
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
			if(fargs->src->type != TYPE_HASHT)
				return -EFAULT;
			hash = fargs->src->ptr;

			if( (dst->ptr = hash_copy(hash)) == NULL)
				return -ENOMEM;
			return 0;

		case FORMAT(packed):
			break;
		
		case FORMAT(human):
		case FORMAT(config):
			dst->ptr = NULL;
			return 0;

		case FORMAT(hash):;
			hash_t                *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if( (dst->ptr = hash_rebuild(config)) == NULL)
				return -EINVAL;
			
			return 0;

		default:
			return -ENOSYS;
	}
	
	hash_nelements = HASH_INITIAL_SIZE;
	hash           = hash_new(hash_nelements);
	
	for(curr = hash;; curr++){
		data_t                d_key          = DATA_PTR_HASHKEYT(&curr->key);
		data_t                d_data         = DATA_PTR_DATAT(&curr->data);
		fastcall_convert_from r_convert      = { { 5, ACTION_CONVERT_FROM }, &sl_src, FORMAT(packed) };
		
		// read key
		if( (ret = data_query(&d_key, &r_convert)) < 0)
			break;
		
		data_slider_t_set_offset(&sl_src, r_convert.transfered, SEEK_CUR);
		
		// set correct 
		if(dst->ptr != NULL){
			if( (old_data = hash_data_find((hash_t *)dst->ptr, curr->key)) != NULL){
				data_t t_data = DATA_PTR_DATAT(old_data);
				d_data = t_data;
			}else{
				data_set_void(&curr->data);
				list_add(&freeit, &curr->data);
			}
		}else{
			data_set_void(&curr->data);
		}
		
		if( (ret = data_query(&d_data, &r_convert)) < 0)
			break;
		
		data_slider_t_set_offset(&sl_src, r_convert.transfered, SEEK_CUR);
		
		if(curr->key == hash_ptr_end)
			break;
		
		nelements++;
		if(nelements + 1 == hash_nelements){
			// expand hash
			hash_t             *new_hash;
			
			hash_nelements *= 2;
			new_hash = hash_new(hash_nelements);
			memcpy(new_hash, hash, nelements * sizeof(hash_t));
			free(hash); // !!! free, not hash_free
			
			curr = new_hash + (curr - hash);
			hash = new_hash;
		}
	}
	
	if(dst->ptr == NULL){
		dst->ptr = (ret == 0) ? hash : NULL; // return hash if status ok
	}else{
		fastcall_free r_free = { { 3, ACTION_FREE } };
		while( (old_data = list_pop(&freeit)) != NULL)
			data_query(old_data, &r_free);
		
		free(hash);      // no need for this hash - we redata data in old one
	}
	
	if(fargs->header.nargs >= 5)
		fargs->transfered = data_slider_t_get_offset(&sl_src);
	
	return ret;
} // }}}

static ssize_t data_hash_t_lookup(data_t *data, fastcall_lookup *fargs){ // {{{
	ssize_t                ret;
	hashkey_t              key_hashkey;
	uintmax_t              key_uint;
	data_t                 d_key;
	data_t                *d_key_ptr         = &d_key;
	hash_t                *value;
	hash_t                *fdata             = (hash_t *)data->ptr;
	
	if(fargs->key == NULL || fargs->value == NULL)
		return -EINVAL;
	
	if(helper_key_current(fargs->key, &d_key) < 0)
		d_key_ptr = fargs->key;
	
	data_get(ret, TYPE_HASHKEYT, key_hashkey, d_key_ptr);
	if(ret == 0){
		if( (value = hash_find(fdata, key_hashkey)) == NULL)
			return -ENOENT;
		
		goto found;
	}
	data_get(ret, TYPE_UINTT, key_uint, d_key_ptr);
	if(ret == 0){
		if( (value = hash_find_id(fdata, &key_uint)) == NULL)
			return -ENOENT;
		
		goto found;
	}
	return -EINVAL;
found:
	return data_notconsumable_t(fargs->value, value->data);
} // }}}
static ssize_t data_hash_t_delete(data_t *data, fastcall_delete *fargs){ // {{{
	ssize_t                ret;
	hashkey_t              key;
	hash_t                *hitem;
	hash_t                *fdata             = (hash_t *)data->ptr;
	
	if(fargs->key == NULL)
		return -EINVAL;
	
	data_get(ret, TYPE_HASHKEYT, key, fargs->key);
	if(ret != 0)
		return -EINVAL;
	
	if( (hitem = hash_find(fdata, key)) == NULL)
		return -ENOENT;
	
	if(fargs->value)
		*fargs->value = hitem->data;
	
	hash_assign_hash_null(hitem);
	return 0;
} // }}}
static ssize_t data_hash_t_enum(data_t *src, fastcall_enum *fargs){ // {{{
	hash_t_ctx             ctx               = { .hash = src->ptr, .sl_data = fargs->dest, .ret = 0 };
	
	hash_iter(ctx.hash, (hash_iterator)&data_hash_t_enum_iter, &ctx, 0);
	return ctx.ret;
} // }}}

data_proto_t hash_t_proto = {
	.type                   = TYPE_HASHT,
	.type_str               = "hash_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handlers               = {
		[ACTION_FREE]         = (f_data_func)&data_hash_t_free,
		[ACTION_COMPARE]      = (f_data_func)&data_hash_t_compare,
		[ACTION_CONVERT_TO]   = (f_data_func)&data_hash_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_hash_t_convert_from,
		
		[ACTION_LOOKUP]       = (f_data_func)&data_hash_t_lookup,
		[ACTION_DELETE]       = (f_data_func)&data_hash_t_delete,
		[ACTION_ENUM]         = (f_data_func)&data_hash_t_enum,
	}
};

static ssize_t hash_rebuild_iter(hash_t *item, hash_t_rebuild_ctx *ctx){ // {{{
	ssize_t                ret;
	
	ctx->nelements++;
	ctx->hash         = realloc(ctx->hash, sizeof(hash_t) * (ctx->nelements + 1));
	if(ctx->hash == NULL)
		return ITER_BREAK;
	
	ctx->hash[ctx->nelements - 1].key = item->key;
	holder_consume(ret, ctx->hash[ctx->nelements - 1].data, &item->data);
	if(ret != 0)
		return ITER_BREAK;
	
	hash_assign_hash_end(&ctx->hash[ctx->nelements]);
	return ITER_CONTINUE;
} // }}}

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
	ssize_t       ret;
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
		
		holder_copy(ret, &el_new->data, &el->data);
		
		el_new++;
	}
	hash_assign_hash_end(el_new);
	
	return new_hash;
} // }}}
hash_t *           hash_rebuild                 (hash_t *hash){ // {{{
	hash_t_rebuild_ctx ctx = {
		.nelements = 0,
		.hash      = NULL
	};
	
	if(hash_iter(hash, (hash_iterator)&hash_rebuild_iter, &ctx, 0) != ITER_OK){
		return NULL;
	}
	return ctx.hash;
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
hash_t *           hash_append(hash_t *hash, hash_t item){ // {{{
	size_t nelements = hash_nelements(hash);

	hash = realloc(hash, (nelements + 1) * sizeof(hash_t));
	hash_assign_hash_t   (&hash[nelements-1], &item);
	hash_assign_hash_end (&hash[nelements  ]);
	return hash;
} // }}}

// TODO rewrite hash_find
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
hash_t *           hash_find_id                 (hash_t *hash, uintmax_t *key){ // {{{
	register hash_t       *inline_hash;
	
	for(;; hash++, *key = *key - 1){
		if(hash->key == hash_ptr_inline){
			if( (inline_hash = hash_find_id((hash_t *)hash->data.ptr, key)) != NULL)
				return inline_hash;
		}
		
		if(hash->key == hash_ptr_end)
			break;
		
		if(*key == 0)
			return hash;
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
static ssize_t     hash_rename_recursive        (hash_t *hash, hash_t **keys){ // {{{
	register hashkey_t     key;
	
	for(;; hash++){
		if(hash->key == hash_ptr_inline){
			hash_rename_recursive((hash_t *)hash->data.ptr, keys);
			continue;
		}
		
		if(hash->key == hash_ptr_end)
			break;
		
		key = (*keys)->key;
		
		if(key == hash_ptr_end)
			break;
		
		hash->key = key;
		*keys = *keys + 1;
	}
	return 0;
} // }}}
ssize_t            hash_rename                  (hash_t *hash, hash_t *keys){ // {{{
	return hash_rename_recursive(hash, &keys);
} // }}}

data_t *           hash_data_find               (hash_t *hash, hashkey_t key){ // {{{
	hash_t *temp;
	return ((temp = hash_find(hash, key)) == NULL) ?
		NULL : &temp->data;
} // }}}

void               hash_dump                    (hash_t *hash){ // {{{
	data_t                 d_hash            = DATA_PTR_HASHT(hash);
	data_t                 d_stderr          = DATA_FDT(2, 0);
	
	fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_TO }, &d_stderr, FORMAT(debug) };
	data_query(&d_hash, &r_convert);
} // }}}

