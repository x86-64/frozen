#include <libfrozen.h>

/* chunks - public {{{1 */
void *      chunk_alloc (size_t size){
	chunk_t *chunk;
	
	chunk = (chunk_t *)malloc(size + sizeof(chunk_t));
	chunk->size = size;
	chunk->next = NULL;
	
	return (void *)(chunk + 1);
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

buffer_t *  buffer_from_data(void *data, size_t data_size){
	buffer_t *buffer = buffer_alloc();
	void     *chunk  = chunk_alloc(data_size);
	
	memcpy(chunk, data, data_size);
	
	buffer_add_tail_chunk(buffer, chunk);
	
	return buffer;
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

void *      buffer_add_new_head_chunk  (buffer_t *buffer, size_t size){
	void *chunk = chunk_alloc(size);
	
	buffer_add_head_chunk(buffer, chunk);
	return chunk;
}

void *      buffer_add_new_tail_chunk  (buffer_t *buffer, size_t size){
	void *chunk = chunk_alloc(size);
	
	buffer_add_tail_chunk(buffer, chunk);
	return chunk;
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

void *      buffer_seek            (buffer_t *buffer, off_t ptr){
	if(ptr == 0)
		return buffer->head;
	
	buffer_process(buffer, (size_t)ptr, 0,
		do{
			if((offset + chunk_size) > ptr)
				return chunk + (ptr - offset);
		}while(0)
	);
	return NULL;
}

ssize_t     buffer_write           (buffer_t *buffer, off_t write_offset, void *buf, size_t buf_size){
	size_t offset_diff;
	
	buffer_process(
		buffer,
		write_offset + buf_size,
		1,
		do {
			if(write_offset >= offset && offset + size > write_offset){ // first chunk to write
				offset_diff = (write_offset - offset);
				
				memcpy(chunk + offset_diff, buf, size - offset_diff);
			}else if(offset >= write_offset){ // next chunks
				memcpy(chunk, buf + (offset - write_offset), size);
			}
			
		}while(0)
	);
	return (write_offset + buf_size);
}
ssize_t     buffer_read            (buffer_t *buffer, off_t read_offset, void *buf, size_t buf_size){
	ssize_t processed = 0;
	size_t  offset_diff;
	
	buffer_process(
		buffer,
		read_offset + buf_size,
		0,
		do {
			if(read_offset >= offset && offset + size > read_offset){ // first chunk to write
				offset_diff = (read_offset - offset);
				
				memcpy(buf, chunk + offset_diff, size - offset_diff);
				processed += size - offset_diff;
			}else if(offset >= read_offset){ // next chunks
				memcpy(buf + (offset - read_offset), chunk, size);
				processed += size;
			}
		}while(0)
	);
	return processed;
}

/* }}}1 */
