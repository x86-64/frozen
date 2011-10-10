#include <libfrozen.h>
#include <hash_t.h>

static ssize_t data_hash_t_compare_iter(hash_t *hash1_item, hash_t *hash2, void *null){ // {{{
	hash_t                *hash2_item;

	if( (hash2_item = hash_find(hash2, hash1_item->key)) == NULL)
		return ITER_BREAK;
	
	fastcall_compare r_compare = { { 4, ACTION_COMPARE }, &hash2_item->data };
	if(data_query(&hash1_item->data, &r_compare) != 0)
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

data_proto_t hash_t_proto = {
	.type                   = TYPE_HASHT,
	.type_str               = "hash_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_COPY]    = (f_data_func)&data_hash_t_copy,
		[ACTION_FREE]    = (f_data_func)&data_hash_t_free,
		[ACTION_COMPARE] = (f_data_func)&data_hash_t_compare,
	}
};
