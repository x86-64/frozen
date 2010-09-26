#include <libfrozen.h>

#define HASH_MAX_ELEMENTS      1000
#define HASH_DATA_BLOCK_SIZE   1000

// - Local hash stored in buffers allocated by our handlers, so we care of free() memory
// - Network hash received from buffer_t, and we dont care about allocation, but
//   hash_t structure is still allocated by us, so we free only it 
// - There is some limitation about hashes for network usage: try not use local hashes
//   after heavy hash_set, coz hash_set on already existing element with data larger than in
//   hash will produce dead data chunk. It still will work, but suffer traffic overhead

hash_t *           hash_new                     (void){ // {{{
	hash_t     *hash;
	hash_el_t  *elements;
	data_t     *data;
	
	hash     = malloc( sizeof(hash_t) );
	if(hash == NULL)
		goto cleanup1;
	
	data     = malloc( HASH_DATABLOCK_SIZE );
	if(data == NULL)
		goto cleanup2;
	
	hash->is_local   = 1;
	hash->nelements  = 0;
	hash->size       = HASH_T_NETWORK_SIZE + nelements * sizeof(hash_el_t);
	hash->alloc_size = HASH_DATABLOCK_SIZE;
	hash->elements   = NULL;
	hash->data       = data;
	hash->data_end   = data;
	
	return hash;
cleanup2:
	free(hash);
cleanup1:
	return NULL;
} // }}}
void               hash_free                    (hash_t *hash){
	if(hash->is_local == 1){
		free(hash->elements);
		free(hash->data);
	}
	free(hash);
}

int                hash_to_buffer               (hash_t  *hash, buffer_t *buffer);
int                hash_from_buffer             (hash_t **hash, buffer_t *buffer);

static unsigned int hash_add_data_chunk (hash_t *hash, void *data, size_t size){
	unsigned int  new_alloc_size;
	unsigned int  new_ptr;
	data_t       *new_data;
	
	new_alloc_size = (hash->data_end - hash->data) + size;
	if( new_alloc_size >= hash->alloc_size){
		new_data = realloc(hash->data, new_alloc_size + HASH_DATABLOCK_SIZE);
		if(new_data == NULL)
			return -1;
		
		hash->data_end = (new_data + (hash->data_end - hash->data));
		hash->data     =  new_data;
	}
	memcpy(hash->data_end, data, size);
	new_ptr          = (hash->data_end - hash->data); 
	hash->data_end  += size;
	hash->size      += size;
	
	return new_ptr;
}

static hash_el_t * hash_search_key              (hash_t *hash, char *key, data_type  type){
	
}

static data_t *    hash_off_to_ptr              (hash_t *hash, unsigned int offset){
	
}

int                hash_set                     (hash_t *hash, char *key, data_type  type, data_t  *value){ // {{{
	unsigned int  new_id;
	unsigned int  new_nelements;
	hash_el_t    *new_elements;
	hash_el_t    *found_key;
	data_t       *found_data;
	size_t        found_size;
	size_t        found_ess_size;
	size_t        value_size;
	
	value_size = data_len(type, value);
	
	found_key = hash_search_key(hash, key, type);
	if(found_key == NULL)
		goto bad;
		
	found_data = hash_off_to_ptr(hash, found_key->data);
	if(found_data == NULL)
		goto bad;
	
	found_ess_size = data_len(found_key->type, NULL); // returns essential size (minimum)
	if(!hash_is_valid_buf(hash, found_data, found_ess_size))
		goto bad;
	
	found_size = data_len(found_key->type, found_data);
	if(!hash_is_valid_buf(hash, found_data, found_size))
		goto bad;
	
	if(found_size >= value_size){
		memcpy(found_data, value, value_size);
		
		return 0;
	}
	
bad:	
	if(hash->is_local == 1){
		new_id        = hash->new_nelements;
		new_nelements = hash->nelements + 1;
		if(new_nelements > HASH_MAX_ELEMENTS)
			return -ENOMEM;
		
		new_elements = realloc(hash->elements, new_nelements * sizeof(hash_el_t));
		if(new_elements == NULL)
			return -ENOMEM;
		hash->elements = new_elements;
		
		hash->elements[new_id].type  = type;
		hash->elements[new_id].key   = hash_add_data_chunk(hash, key,   strlen(key) + 1);
		hash->elements[new_id].value = hash_add_data_chunk(hash, value, value_size);
		
		return 0;
	}
	
	return -1;
} // }}}

data_t *           hash_get                     (hash_t *hash, char *key, data_type  type);
int                hash_get_copy                (hash_t *hash, char *key, data_type  type, data_t  *buf);
int                hash_get_any                 (hash_t *hash, char *key, data_type *type, data_t **value);

int                hash_is_valid_buf            (hash_t *hash, data_t *data, unsigned int size);


/*
// never use hash->size as buffer_size argument, it can lead to security problems
int                hash_audit                   (hash_t *hash, size_t buffer_size){
	unsigned int   looked_size;
	unsigned int   hash_size = hash->size;
	hash_el_t     *elements;
	
	// validate size
	if(hash_size > buffer_size)
		return -EINVAL;
	
	elements = (hash_el_t *)(hash + 1);
	
	// search for 0xFF slot
	looked_size = sizeof(hash_t);
	while(looked_size + sizeof(hash_el_t) <= hash_size){
		if(is_set(elements, 0xFF, sizeof(hash_el_t)) != 0){
			return 0;
		}
		
		elements++;
		looked_size += sizeof(hash_el_t);
	}
	return -1;
}

int                hash_is_valid_buf            (hash_t *hash, data_t *data, unsigned int size){
	if(size >= hash->size)
		return 0;
	if((void *)data <= (void *)hash || (void *)data >= (void *)hash + hash->size)
		return 0;
	if((void *)data + size > (void *)hash + hash->size)
		return 0;
	return 1;
}

int                hash_get                     (hash_t *hash, char *key, data_type *type, data_t **data){
	hash_el_t    *curr;
	hash_el_t    *elements;
	char         *key_hash;
	char         *key_args;
	char         *hash_data_ptr;
	char         *hash_data_end;
	unsigned int  hash_data_len;
	
	if(hash == NULL || key == NULL || type == NULL || data == NULL)
		return -EINVAL;
	
	elements = (hash_el_t *)(hash + 1);
	
	for(curr = elements; is_set(curr, 0xFF, sizeof(hash_el_t)) == 0; curr++);
	curr++;
	hash_data_ptr = (char *)curr;
	hash_data_end = (char *)hash + hash->size;
	hash_data_len = hash_data_end - hash_data_ptr;
	
	for(curr = elements; is_set(curr, 0xFF, sizeof(hash_el_t)) == 0; curr++){
		if(curr->key >= hash_data_len)
			goto next_element;                                               // broken element's key
		
		key_hash = hash_data_ptr + curr->key;
		key_args = key;
		while(key_hash < hash_data_end){
			if(*key_hash != *key_args)
				break;
			
			if(*key_hash == 0 && *key_args == 0){
				if(!data_type_is_valid( (data_type)curr->type ))
					goto next_element;                               // broken element's type
				
				*type = (data_type)curr->type;
				
				if(curr->value >= hash_data_len)
					goto next_element;                       // broken element's value
					
				*data = (data_t *)(hash_data_ptr + curr->value);
				return 0;
			}
			
			if(*key_hash == 0 || *key_args == 0)
				break;
			key_hash++;
			key_args++;
		}
	next_element:
		;
	}
	return -1;
}

data_t *           hash_get_typed               (hash_t *hash, char *key, data_type type){
	data_type      fetched_type;
	data_t        *fetched_data;
	
	if(
		hash_get(hash, key, &fetched_type, &fetched_data) == 0 &&
		fetched_type == type
	){
		return fetched_data;
	}
	
	return NULL;
}

int                hash_get_in_buf              (hash_t *hash, char *key, data_type type, data_t *buf){
	data_type      fetched_type;
	data_t        *fetched_data;
	size_t         fetched_len;
	
	if(
		hash_get(hash, key, &fetched_type, &fetched_data) == 0 &&
		fetched_type == type
	){
		fetched_len = data_len(fetched_type, fetched_data);
		
		memcpy(buf, fetched_data, fetched_len); 
		return 0;
	}
	return -1;
}
*/
