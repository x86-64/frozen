#include <libfrozen.h>
#include <alloca.h>

hash_t *         hash_find              (hash_t *hash, char *key){ // {{{
	hash_t      *value = hash;
	
	if(hash == NULL)
		return NULL;
	
	while(value->key != hash_ptr_end){
		if(
			value->key != NULL &&
			(
				value->key == key ||
				(
					key != hash_ptr_null && key != hash_ptr_end &&
					value->key != hash_ptr_null &&
					strcmp(value->key, key) == 0
				)
			)
		)
			return value;
		
		value++;
	}
	if(value->data.data_ptr != NULL)
		return hash_find((hash_t *)value->data.data_ptr, key);
	
	return NULL;
} // }}}
hash_t *         hash_find_typed        (hash_t *hash, data_type type, char *key){ // {{{
	hash_t      *hvalue = hash_find(hash, key);
	return (hvalue != NULL) ? ((hvalue->data.type != type) ? NULL : hvalue) : NULL;
} // }}}
hash_t *         hash_set                     (hash_t *hash, char *key, data_type type, void *value, size_t value_size){ // {{{
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
void               hash_delete                (hash_t *hash, char *key){ // {{{
	hash_t       *hvalue;
	
	if( (hvalue = hash_find(hash, key)) == NULL)
		return;
	
	hvalue->key = hash_ptr_null;
} // }}}
void               hash_chain                   (hash_t *hash, hash_t *hash_next){ // {{{
	hash_t *hend = hash_find(hash, hash_ptr_end);
	hend->data.data_ptr = hash_next;
} // }}}
int                hash_iter                    (hash_t *hash, hash_iterator func, void *arg1, void *arg2){ // {{{
	hash_t      *value = hash;
	int          ret;
	
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
int                hash_get                     (hash_t *hash, char *key, data_type *type, void **value, size_t *value_size){ // {{{
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
int                hash_get_typed               (hash_t *hash, data_type type, char *key, void **value, size_t *value_size){ // {{{
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
		
		el_new->key = (el->key) ? strdup(el->key) : NULL;
		data_copy(&el_new->data, &el->data);
		
		el_new++;
	}
	el_new->key           = hash_ptr_end;
	el_new->data.data_ptr = (el->data.data_ptr != NULL) ? hash_copy(el->data.data_ptr) : NULL;
	
	return new_hash;
} // }}}

/** @brief Free hash
 *  @param[in]  hash        Hash to free
 *  @param[in]  recursive   Also free recurive hashes
 */
void               hash_free                    (hash_t *hash, int recursive){ // {{{
	hash_t       *el;
	
	if(hash == NULL)
		return;
	
	for(el = hash; el->key != hash_ptr_end; el++){
		if(el->key == hash_ptr_null)
			continue;
		
		if(el->key)
			free(el->key);
		data_free(&el->data);
	}
	if(recursive != 0)
		hash_free(el->data.data_ptr, recursive);
	
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

data_t *           hash_get_data                (hash_t *hash, char *key){ // {{{
	hash_t *htemp;
	
	if( (htemp = hash_find(hash, key)) == NULL)
		return NULL;
	
	return &htemp->data;
} // }}}
data_t *           hash_get_typed_data          (hash_t *hash, data_type type, char *key){ // {{{
	hash_t *htemp;
	
	if( (htemp = hash_find_typed(hash, type, key)) == NULL)
		return NULL;
	
	return &htemp->data;
} // }}}
data_ctx_t *       hash_get_data_ctx            (hash_t *hash, char *key){ // {{{
	hash_t *htemp;
	char *ctx_str = alloca(strlen(key) + 4 + 1); // TODO remove alloca
	
	strcpy(ctx_str, key);
	strcat(ctx_str, "_ctx");
	
	if( (htemp = hash_find(hash, ctx_str)) != NULL)
		return htemp->data.data_ptr;
	
	return NULL;
} // }}}


#ifdef DEBUG
void hash_dump(hash_t *hash){
	unsigned int  k;
	hash_t       *element = hash;
	
	printf("hash: %p\n", hash);
start:
	while(element->key != hash_ptr_end){
		if(element->key == hash_ptr_null)
			goto next_item;
		
		printf(" - %s [%s] -> %p", (char *)element->key, data_string_from_type(element->data.type), element->data.data_ptr);
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
}
#endif

