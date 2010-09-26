#include <libfrozen.h>

#define MAX_HASH_ELEMENTS         10000
#define HASH_BUILDER_BLOCK_SIZE   1000

// TODO hash_resize(hash_builder_t **builder, unsigned int nelements);

int                is_set                       (void *memory, char chr, size_t size){
	char *p = (char *)memory;
	while(*p++ == chr && --size > 0);
	
	return (size == 0);
}

hash_builder_t *   hash_builder_new             (unsigned int nelements){
	size_t    alloc_size;
	hash_t   *hash;
	size_t    hash_size;
	void     *data_ptr;
	char     *p;
	
	if(nelements > MAX_HASH_ELEMENTS)
		return NULL;
	
	hash_size  = (nelements + 1) * sizeof(hash_el_t) + sizeof(hash_t); 
	alloc_size = hash_size + HASH_BUILDER_BLOCK_SIZE;
	
	hash_builder_t *builder = (hash_builder_t *)malloc(sizeof(hash_builder_t) + alloc_size);
	if(builder == NULL)
		return NULL;
	
	memset(builder, 0, sizeof(hash_builder_t) + alloc_size);
	
	p           = (char *)builder;
	
	p          += sizeof(hash_builder_t);
	hash        = (hash_t *)p;
	hash->size  = hash_size;
	
	p          += sizeof(hash_t);
	p          += nelements * sizeof(hash_el_t);
	memset(p, 0xFF, sizeof(hash_el_t));          // last element
	
	p          += sizeof(hash_el_t);
	data_ptr    = p;
	
	builder->alloc_size    = alloc_size;
	builder->nelements     = nelements;
	builder->hash          = hash;
	builder->data_ptr      = data_ptr;
	builder->data_last_off = 0;
	
	return builder;
}

static unsigned int hash_builder_add_data_chunk (hash_builder_t **p_builder, void *data, size_t size){
	unsigned int     res;
	hash_builder_t  *builder  = *p_builder;
	
	if(builder->hash->size + size > builder->alloc_size){
		builder->alloc_size  = builder->hash->size + size + HASH_BUILDER_BLOCK_SIZE;
		
		*p_builder = builder = realloc(builder, builder->alloc_size);
		if(builder == NULL)
			return 0;
		
		builder->hash        = (hash_t *)(builder + 1); // sizeof(hash_builder_t)
		builder->data_ptr    = (char *)builder->hash + sizeof(hash_t) + (builder->nelements + 1) * sizeof(hash_el_t);
	}
	
	memcpy(builder->data_ptr + builder->data_last_off, data, size);
	
	res = builder->data_last_off;
	
	builder->data_last_off += size;
	builder->hash->size    += size;
	
	return res;
}

int                 hash_builder_add_data       (hash_builder_t **p_builder, char *key, data_type type, data_t *value){
	hash_builder_t  *builder = *p_builder;
	hash_el_t       *elements;
	unsigned int     hash_id;
	unsigned int     key_chunk;
	unsigned int     val_chunk;
	size_t           val_size;
	
	if(p_builder == NULL || key == NULL || value == NULL)
		return -EINVAL;
	
	elements = (hash_el_t *)(builder->hash + 1); // sizeof(hash_t)
	
	// find free slot
	hash_id = 0;
	while( is_set(&elements[hash_id], 0x0, sizeof(hash_el_t)) == 0)
		hash_id++;
	
	if(hash_id >= builder->nelements)
		return -ENOMEM; // TODO call hash_resize
	
	val_size  = data_len(type, value);
	
	key_chunk = hash_builder_add_data_chunk(p_builder, key,   strlen(key) + 1);
	val_chunk = hash_builder_add_data_chunk(p_builder, value, val_size);
	
	builder  = *p_builder;
	elements = (hash_el_t *)(builder->hash + 1); // sizeof(hash_t)
	elements[hash_id].key   = key_chunk;
	elements[hash_id].type  = (unsigned int)type;
	elements[hash_id].value = val_chunk;
	
	return 0;
}

hash_t *           hash_builder_get_hash        (hash_builder_t *builder){
	return builder->hash;
}

void               hash_builder_free            (hash_builder_t *builder){
	free(builder);
}


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

