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

void               hash_dump                    (hash_t *hash){ // {{{
	char                   buffer[DEF_BUFFER_SIZE*20];
	data_t                 d_hash            = DATA_PTR_HASHT(hash);
	data_t                 d_buffer          = DATA_RAW(buffer, sizeof(buffer));
	
	fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_TO }, &d_buffer, FORMAT(debug) };
	if(data_query(&d_hash, &r_convert) < 0)
		return;
	
	fprintf(stderr, "%s", buffer);
} // }}}

