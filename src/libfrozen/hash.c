#include <libfrozen.h>

#define DEBUG

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
void               hash_assign                  (hash_t *hash, hash_t *hash_next){ // {{{
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
	char *ctx_str = alloca(strlen(key) + 4 + 1);
	
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
	while(element->key != hash_ptr_end){
		if(element->key == hash_ptr_null)
			continue;
		
		printf(" - el: %x %s -> %p", (unsigned int)element->data.type, (char *)element->key, element->data.data_ptr);
		for(k=0; k<element->data.data_size; k++){
			if((k % 8) == 0)
				printf("\n   0x%.4x: ", k);
			
			printf("%.2x ", (unsigned int)(*((char *)element->data.data_ptr + k)));
		}
		printf("\n");
		element++;
	}
	printf("end_hash\n");
}
#endif

