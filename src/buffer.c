#include <libfrozen.h>

/* chunks - public {{{1 */
void *      chunk_alloc (size_t size){
	chunk_t *chunk;
	
	chunk = (chunk_t *)malloc(size + sizeof(chunk_t));
	chunk->size = size;
	chunk->next = NULL;
	
	return (void *)(chunk + 1);
}

size_t      chunk_get_size(void *chunk){
	return ( ((chunk_t *)chunk) - 1)->size;
}

void *      chunk_next  (void *chunk){
	return ( ((chunk_t *)chunk) - 1)->next;
}

void        chunk_free  (void *chunk){
	free( ((chunk_t *)chunk) - 1);
}
/* }}}1 */
/* buffers - public {{{1 */
void        buffer_init    (buffer_t *buffer){
	//buffer->size = 0;
	//buffer->head = NULL;
	//buffer->tail = NULL;
	memset(buffer, 0, sizeof(buffer_t));
}

void        buffer_remove_chunks(buffer_t *buffer){
	void *chunk = buffer->head;
	void *chunkn;
	while(chunk != NULL){
		chunkn = chunk_next(chunk);
		chunk_free(chunk);
		chunk = chunkn;
	}
	buffer_init(buffer);
}

void        buffer_destroy (buffer_t *buffer){
	buffer_remove_chunks(buffer);
}

buffer_t *  buffer_alloc   (void){
	buffer_t *buffer = (buffer_t *)malloc(sizeof(buffer_t));
	buffer_init(buffer);
	return buffer;
}

void        buffer_free    (buffer_t *buffer){
	buffer_destroy(buffer);
	free(buffer);
}

void        buffer_add_head_chunk (buffer_t *buffer, void *chunk){
	size_t size = chunk_get_size(chunk);
	buffer->size += size;
	
	if(buffer->head == NULL){
		buffer->tail = chunk;
	}else{
		((chunk_t *)chunk - 1)->next = buffer->head;
	}
	buffer->head = chunk;
}

void        buffer_add_tail_chunk (buffer_t *buffer, void *chunk){
	size_t size = chunk_get_size(chunk);
	buffer->size += size;
	
	if(buffer->tail == NULL){
		buffer->head = chunk;
	}else{
		((chunk_t *)buffer->tail - 1)->next = chunk;
	}
	buffer->tail = chunk;
}

int         buffer_delete_chunk   (buffer_t *buffer, void *chunk){
	int     ret = -1;
	size_t  size;
	void   *curr, *last;
	
	last = NULL;
	curr = buffer->head;
	while(curr != NULL){
		if(curr == chunk){
			if(last != NULL)
				( (chunk_t *)last - 1)->next = ( (chunk_t *)curr - 1)->next;
			if(buffer->tail == curr)
				buffer->tail = last;
			if(buffer->head == curr)
				buffer->head = ( (chunk_t *)curr - 1)->next;
			ret = 0;
			break;
		}
		last = curr;
		curr = ( (chunk_t *)curr - 1)->next;
	}
	if(ret == 0){
		size = chunk_get_size(chunk);
		buffer->size -= size;
		
		chunk_free(chunk);
	}
	return ret;
}

size_t      buffer_get_size       (buffer_t *buffer){
	return buffer->size;
}

void *      buffer_get_first_chunk(buffer_t *buffer){
	return buffer->head;
}
/*
buffer_t *  buffer_prealloced(size_t data_size){
	buffer_t *buffer = buffer_alloc();
	void     *chunk  = chunk_alloc(data_size);

	return buffer;
}*/

buffer_t *  buffer_from_data(void *data, size_t data_size){
	buffer_t *buffer = buffer_alloc();
	void     *chunk  = chunk_alloc(data_size);
	
	memcpy(chunk, data, data_size);
	
	buffer_add_tail_chunk(buffer, chunk);
	
	return buffer;
}
/*
int         buffer_cmp(buffer_t *buffer1, buffer_t *buffer2){
	void *chunk1 = buffer_get_first_chunk(buffer1);
	void *chunk2 = buffer_get_first_chunk(buffer2);
	size_t chunk1_size = chunk_get_size(chunk1);
	size_t chunk2_size = chunk_get_size(chunk2);
	size_t size;
	
	size = (chunk1_size > chunk2_size) ? chunk2_size : chunk1_size;
	
	return memcmp(chunk1, chunk2, size);
}*/

/* }}}1 */
