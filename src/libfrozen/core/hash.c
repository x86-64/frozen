#define HASH_C
#include <libfrozen.h>
#include <hashkeys_int.h>

// TODO recursive hashes in to\from_memory

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
static int hash_bsearch_string(const void *m1, const void *m2){ // {{{
	hash_keypair_t *mi1 = (hash_keypair_t *) m1;
	hash_keypair_t *mi2 = (hash_keypair_t *) m2;
	return strcmp(mi1->key_str, mi2->key_str);
} // }}}
static int hash_bsearch_int(const void *m1, const void *m2){ // {{{
	hash_keypair_t *mi1 = (hash_keypair_t *) m1;
	hash_keypair_t *mi2 = (hash_keypair_t *) m2;
	return (mi1->key_val - mi2->key_val);
} // }}}
static ssize_t hash_to_buffer_one(hash_t *hash, void *p_buffer, void *p_null){ // {{{
	void     *data_ptr;
	size_t    data_size;
	
	data_ptr  = data_value_ptr(&hash->data);
	data_size = data_value_len(&hash->data);
	
	if(data_ptr == NULL || data_size == 0)
		return ITER_CONTINUE;
	
	buffer_add_tail_raw((buffer_t *)p_buffer, data_ptr, data_size);
	return ITER_CONTINUE;
} // }}}

hash_key_t         hash_string_to_key           (char *string){ // {{{
	hash_keypair_t  key, *ret;
	key.key_str = string;
	
	if(string == NULL)
		return 0;
	
	/*
	// get valid ordering
	qsort(hash_keys, hash_keys_nelements, hash_keys_size, &hash_bsearch_string);
	int i;
	for(i=0; i<hash_keys_nelements; i++){
		printf("REGISTER_KEY(%s)\n", hash_keys[i].key_str);
	}
	exit(0);
	*/

	if((ret = bsearch(&key, hash_keys,
		hash_keys_nelements, hash_keys_size,
		&hash_bsearch_string)) == NULL)
		return 0;
	
	return ret->key_val;
} // }}}
char *             hash_key_to_string           (hash_key_t key_val){ // {{{
	hash_keypair_t  key, *ret;
	key.key_val = key_val;
	
	if(key_val == 0)
		goto ret_null;
	
	if((ret = bsearch(&key, hash_keys,
		hash_keys_nelements, hash_keys_size,
		&hash_bsearch_int)) == NULL)
		goto ret_null;
	
	return ret->key_str;
ret_null:
	return "(null)";
} // }}}
hash_key_t         hash_key_to_ctx_key          (hash_key_t key){ // {{{
	return key + HASH_CTX_KEY_OFFSET;
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
		
		el_new->key = el->key;
		data_copy(&el_new->data, &el->data);
		
		el_new++;
	}
	el_new->key           = hash_ptr_end;
	el_new->data.data_ptr = (el->data.data_ptr != NULL) ? hash_copy(el->data.data_ptr) : NULL;
	
	return new_hash;
} // }}}
void               hash_free                    (hash_t *hash){ // {{{
	hash_t       *el;
	
	if(hash == NULL)
		return;
	
	for(el = hash; el->key != hash_ptr_end; el++){
		if(el->key == hash_ptr_null)
			continue;
		
		data_free(&el->data);
	}
	hash_free(el->data.data_ptr);
	
	free(hash);
} // }}}

hash_t *           hash_find                    (hash_t *hash, hash_key_t key){ // {{{
	for(; hash != NULL; hash = (hash_t *)hash->data.data_ptr){
		goto loop_start;
		do{
			hash++;

		loop_start:
			if(hash->key == key)
				return hash;
		}while(hash->key != hash_ptr_end);
	}
	return NULL;
} // }}}
ssize_t            hash_iter                    (hash_t *hash, hash_iterator func, void *arg1, void *arg2){ // {{{
	hash_t      *value = hash;
	ssize_t      ret;
	
	if(hash == NULL)
		return ITER_BREAK;
	
	while(value->key != hash_ptr_end){
		if( value->key == hash_ptr_null )
			goto next;
		
		ret = func(value, arg1, arg2);
		if(ret != ITER_CONTINUE)
			return ret;
	
	next:	
		value++;
	}
	if(value->data.data_ptr != NULL)
		return hash_iter((hash_t *)value->data.data_ptr, func, arg1, arg2);
	
	return ITER_OK;
} // }}}
void               hash_chain                   (hash_t *hash, hash_t *hash_next){ // {{{
	hash_t *hend;
	do{
		hend = hash_find(hash, hash_ptr_end);
	}while( (hash = hend->data.data_ptr) != NULL );
	hend->data.data_ptr = hash_next;
} // }}}
void               hash_unchain                 (hash_t *hash, hash_t *hash_unchain){ // {{{
	hash_t *hend;
	do{
		hend = hash_find(hash, hash_ptr_end);
	}while( (hash = hend->data.data_ptr) != hash_unchain );
	hend->data.data_ptr = NULL;
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
	
	// TODO remake in streaming style (read)
	// TODO SEC validate
	
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
		
		data_size = data_value_len(&curr->data);
		
		if(data_size > size)
			goto error;
		
		curr->data->type = data_value_type(&curr->data);
		curr->data->ptr  = ptr;
		
		data_off += data_size;
	}	
	
	*hash = (hash_t *)memory;
	return 0;
error:
	return -EFAULT;
} // }}}
/*
ssize_t            hash_to_memory               (hash_t  *hash, void *memory, size_t memory_size){ // {{{
	size_t  i, nelements, hash_size;
	
	nelements = hash_nelements(hash);
	hash_size = nelements * sizeof(hash_t);
	
	_hash_to_memory_cpy(hash, hash_size);
	for(i=0; i<nelements; i++, hash++){
		if(hash->data.data_ptr == NULL || hash->data.data_size == 0)
			continue;
		
		_hash_to_memory_cpy(hash->data.data_ptr, hash->data.data_size);
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
		if(hash->data.data_ptr == NULL || hash->data.data_size == 0)
			continue;
		if(hash->key == hash_ptr_null)
			continue;
		
		_hash_from_memory_cpy(hash->data.data_ptr, hash->data.data_size);
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
		
		data_size = curr->data.data_size; //data_value_len(&curr->data);
		
		if(data_size > memory_size)
			goto error;
		
		curr->data.data_ptr = memory + data_off;
		//data_assign_raw(
		//	&curr->data,
		//	data_value_type(&curr->data),
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

inline hash_key_t         hash_item_key                (hash_t *hash){ return hash->key; }
inline size_t             hash_item_is_null            (hash_t *hash){ return (hash->key == hash_ptr_null); }
inline data_t *           hash_item_data               (hash_t *hash){ return &(hash->data); }
inline hash_t *           hash_item_next               (hash_t *hash){ return ((hash+1)->key == hash_ptr_end) ? NULL : hash + 1; }
inline void               hash_data_find               (hash_t *hash, hash_key_t key, data_t **data){
	hash_t *temp;
	if(data){
		*data     =
			((temp = hash_find(hash, key                     )) == NULL) ?
			NULL : hash_item_data(temp);
	}
}

#ifdef DEBUG
void hash_dump(hash_t *hash){ // {{{
	unsigned int  k;
	hash_t       *element = hash;
	
	printf("hash: %p\n", hash);
start:
	while(element->key != hash_ptr_end){
		if(element->key == hash_ptr_null){
			printf(" - hash_null\n");
			goto next_item;
		}
		
		printf(" - %s [%s] -> %p", hash_key_to_string(element->key), data_string_from_type(element->data.type), element->data.data_ptr);
		for(k=0; k<element->data.data_size; k++){
			if((k % 32) == 0)
				printf("\n   0x%.5x: ", k);
			
			printf("%.2hhx ", (unsigned int)(*((char *)element->data.data_ptr + k)));
		}
		printf("\n");
	
	next_item:
		element++;
	}
	printf("end_hash\n");
	if(element->data.data_ptr != NULL){
		printf("recursive hash: %p\n", element->data.data_ptr);
		element = element->data.data_ptr;
		goto start;
	}
} // }}}
#endif

