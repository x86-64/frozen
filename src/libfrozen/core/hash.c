#include <libfrozen.h>

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
		
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &el_new->data };
		data_query(&el->data, &r_copy);
		
		el_new++;
	}
	el_new->key      = hash_ptr_end;
	el_new->data.ptr = (el->data.ptr != NULL) ? hash_copy(el->data.ptr) : NULL;
	
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
	hash_free(el->data.ptr);
	
	free(hash);
} // }}}

hash_t *           hash_find                    (hash_t *hash, hash_key_t key){ // {{{
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

			if( (ret = hash_iter((hash_t *)curr->data.ptr, func, arg, flags)) != ITER_OK)
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
	
	// hash_next
	if(curr->data.ptr != NULL)
		return hash_iter((hash_t *)curr->data.ptr, func, arg, flags);
	
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

data_t *           hash_data_find               (hash_t *hash, hash_key_t key){ // {{{
	hash_t *temp;
	return ((temp = hash_find(hash, key)) == NULL) ?
		NULL : &temp->data;
} // }}}

#ifdef DEBUG
static void hash_dump_pad(uintmax_t reclevel){
	for(;reclevel > 0; reclevel--)
		printf("\t");
}

static ssize_t hash_dump_iter(hash_t *element, uintmax_t *reclevel){
	unsigned int           k;
	data_t                 d_s_key  = DATA_STRING(NULL);
	data_t                 d_s_type = DATA_STRING(NULL);
	
	if(element->key == hash_ptr_null){
		hash_dump_pad(*reclevel);
		printf(" - hash_null\n");
		return ITER_CONTINUE;
	}
	if(element->key == hash_ptr_end){
		hash_dump_pad(*reclevel);
		printf(" - hash_end\n");
		
		if(*reclevel > 0)
			*reclevel -= 1;
		return ITER_CONTINUE;
	}
	if(element->key == hash_ptr_inline){
		hash_dump_pad(*reclevel);
		printf(" - hash_inline:\n");
		
		*reclevel += 1;
		return ITER_CONTINUE;
	}

	data_t              d_key     = DATA_HASHKEYT(element->key);
	fastcall_convert_to r_convert1 = { { 4, ACTION_CONVERT_TO }, &d_s_key,  FORMAT_CLEAN };
	data_query(&d_key, &r_convert1);
	
	data_t              d_type    = DATA_DATATYPET(element->data.type);
	fastcall_convert_to r_convert2 = { { 4, ACTION_CONVERT_TO }, &d_s_type, FORMAT_CLEAN };
	data_query(&d_type, &r_convert2);

	hash_dump_pad(*reclevel);
	printf(" - %s [%s] -> %p", (char *)d_s_key.ptr, (char *)d_s_type.ptr, element->data.ptr);
	
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&d_s_key,  &r_free);
	data_query(&d_s_type, &r_free);

	fastcall_getdataptr r_ptr = { { 3, ACTION_GETDATAPTR } };
	data_query(&element->data, &r_ptr);
	fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
	data_query(&element->data, &r_len);

	for(k = 0; k < r_len.length; k++){
		if((k % 32) == 0){
			printf("\n");
			hash_dump_pad(*reclevel);
			printf("   0x%.5x: ", k);
		}
		
		printf("%.2hhx ", (unsigned int)(*((char *)r_ptr.ptr + k)));
	}
	printf("\n");
	return ITER_CONTINUE;
}

void hash_dump(hash_t *hash){ // {{{
	uintmax_t              reclevel = 0;
	
	hash_iter(hash, (hash_iterator)&hash_dump_iter, &reclevel, HASH_ITER_NULL | HASH_ITER_INLINE | HASH_ITER_END);
} // }}}
#endif

