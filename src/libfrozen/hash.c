#define HASH_C
#include <libfrozen.h>
#include <alloca.h>

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

hash_t *           hash_find              (hash_t *hash, hash_key_t key){ // {{{
	if(hash == NULL)
		return NULL;
	do{
		if(hash->key == key){
			return hash;
		}else if(hash->key == hash_ptr_end){
			goto end;
		}else{
			hash++;
		}
	}while(1);
end:
	if(hash->data.data_ptr != NULL)
		return hash_find((hash_t *)hash->data.data_ptr, key);
	
	return NULL;
} // }}}
hash_t *           hash_find_typed        (hash_t *hash, data_type type, hash_key_t key){ // {{{
	hash_t *hvalue = hash_find(hash, key);
	return (hvalue != NULL) ? ((hvalue->data.type != type) ? NULL : hvalue) : NULL;
} // }}}
hash_t *           hash_set                     (hash_t *hash, hash_key_t key, data_type type, void *value, size_t value_size){ // {{{
	hash_t      *hvalue;
	
	if( (hvalue = hash_find(hash, key)) == NULL){
		if( (hvalue = hash_find(hash, hash_ptr_null)) == NULL)
			return NULL;
	}
	
	hvalue->key            = key;
	hvalue->data.type      = type;
	hvalue->data.data_ptr  = value;
	hvalue->data.data_size = value_size;
	return hvalue;
} // }}}
void               hash_delete                (hash_t *hash, hash_key_t key){ // {{{
	hash_t       *hvalue;
	
	if( (hvalue = hash_find(hash, key)) == NULL)
		return;
	
	hvalue->key = hash_ptr_null;
} // }}}
void               hash_chain                   (hash_t *hash, hash_t *hash_next){ // {{{
	hash_t *hend = hash_find(hash, hash_ptr_end);
	hend->data.data_ptr = hash_next;
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
data_type          hash_get_data_type           (hash_t *hash){ // {{{
	return hash->data.type;
} // }}}
void *             hash_get_value_ptr           (hash_t *hash){ // {{{
	return hash->data.data_ptr;
} // }}}
size_t             hash_get_value_size          (hash_t *hash){ // {{{
	return hash->data.data_size;
} // }}}
int                hash_get                     (hash_t *hash, hash_key_t key, data_type *type, void **value, size_t *value_size){ // {{{
	hash_t *hvalue;
	
	if( (hvalue = hash_find(hash, key)) == NULL)
		return -1;
	
	if(type != NULL)
		*type = hvalue->data.type;
	if(value != NULL)
		*value = hvalue->data.data_ptr;
	if(value_size != NULL)
		*value_size = hvalue->data.data_size;
	
	return 0;
} // }}}
int                hash_get_typed               (hash_t *hash, data_type type, hash_key_t key, void **value, size_t *value_size){ // {{{
	hash_t *hvalue;
	
	if( (hvalue = hash_find_typed(hash, type, key)) == NULL)
		return -1;
	
	if(value != NULL)
		*value = hvalue->data.data_ptr;
	if(value_size != NULL)
		*value_size = hvalue->data.data_size;
	
	return 0;
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

size_t             hash_get_nelements           (hash_t *hash){ // {{{
	hash_t       *el;
	unsigned int  i;
	
	if(hash == NULL)
		return 0;
	
	for(el = hash, i=0; el->key != hash_ptr_end; el++){
		i++;
	}
	return i + 1;
} // }}}

data_t *           hash_get_data                (hash_t *hash, hash_key_t key){ // {{{
	hash_t *htemp;
	return ((htemp = hash_find(hash, key)) != NULL) ? &htemp->data : NULL;
} // }}}
data_t *           hash_get_typed_data          (hash_t *hash, data_type type, hash_key_t key){ // {{{
	hash_t *htemp;
	return ((htemp = hash_find_typed(hash, type, key)) != NULL) ? &htemp->data : NULL;
} // }}}
data_ctx_t *       hash_get_data_ctx            (hash_t *hash, hash_key_t key){ // {{{
	/*
	hash_t *htemp;
	char *ctx_str = alloca(strlen(key) + 4 + 1); // TODO remove alloca
	
	strcpy(ctx_str, key);
	strcat(ctx_str, "_ctx");
	
	if( (htemp = hash_find(hash, ctx_str)) != NULL)
		return htemp->data.data_ptr;
	*/
	// TODO IMPORTANT
	return NULL;
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

ssize_t            hash_to_buffer               (hash_t  *hash, buffer_t *buffer){ // {{{
	size_t  nelements;
	
	buffer_init(buffer);
	
	nelements = hash_get_nelements(hash);
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
		
		data_assign_raw(
			&curr->data,
			data_value_type(&curr->data),
			ptr,
			data_size
		);
		data_off += data_size;
	}	
	
	*hash = (hash_t *)memory;
	return 0;
error:
	return -EFAULT;
} // }}}

#ifdef DEBUG
void hash_dump(hash_t *hash){ // {{{
	unsigned int  k;
	hash_t       *element = hash;
	
	printf("hash: %p\n", hash);
start:
	while(element->key != hash_ptr_end){
		if(element->key == hash_ptr_null)
			goto next_item;
		
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

