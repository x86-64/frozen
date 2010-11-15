/**
 * @file buffer.c
 * @ingroup buffer
 * @brief Working with buffer_t
 */

#include <libfrozen.h>

#define MEMCMP_BUFF_SIZE 1024

static void     buffer_add_head_any        (buffer_t *buffer, void *chunk);
static void     buffer_add_tail_any        (buffer_t *buffer, void *chunk);

/* chunks - public {{{ */
void *      chunk_data_alloc (size_t size){ // {{{
	chunk_t *chunk;

	chunk = malloc(size + sizeof(chunk_t));
	chunk->type = CHUNK_TYPE_ALLOCATED;
	chunk->size = size;
	chunk->next = NULL;

	return (void *)(chunk + 1);
} // }}}
void *      chunk_bare_alloc (void *ptr, size_t size){ // {{{
	chunk_t *chunk;

	chunk = malloc(sizeof(chunk_t));
	chunk->type     = CHUNK_TYPE_BARE;
	chunk->bare_ptr = ptr;
	chunk->size     = size;
	chunk->next     = NULL;

	return (void *)(chunk + 1);
} // }}}
inline size_t chunk_get_size (void *chunk){ return ( ((chunk_t *)chunk) - 1)->size; }
inline void * chunk_get_ptr  (void *chunk){
	switch( ( ((chunk_t *)chunk) - 1)->type ){
		case CHUNK_TYPE_ALLOCATED:
			return chunk;
		case CHUNK_TYPE_BARE:
			return ( ((chunk_t *)chunk) - 1)->bare_ptr;
	};
	return NULL;
}
inline void * chunk_next     (void *chunk){ return ( ((chunk_t *)chunk) - 1)->next; }
inline void   chunk_free     (void *chunk){ free( ((chunk_t *)chunk) - 1); }

/* }}} */
/* buffers - public {{{1 */

/** @brief Initialize buffer
 *  @param buffer  Place for buffer
 */
void            buffer_init                (buffer_t *buffer){ // {{{
	memset(buffer, 0, sizeof(buffer_t));
	buffer->type = BUFF_TYPE_CHUNKED;
} // }}}

/** @brief Allocate new buffer
 *  @return buffer_t * or NULL
 */
buffer_t *      buffer_alloc               (void){ // {{{
	buffer_t *buffer = (buffer_t *)malloc(sizeof(buffer_t));
	buffer_init(buffer);
	return buffer;
} // }}}

/** @brief Associate buffer with read and write functions
 *  @param[out] buffer   Place to write buffer structure
 *  @param[in]  context  IO context for functions
 *  @param[in]  read     IO read function
 *  @param[in]  write    IO write function
 *  @param[in]  cached   Cache returned io buffers
 */
void            buffer_io_init             (buffer_t *buffer, void *context, func_buffer read, func_buffer write, int cached){ // {{{
	memset(buffer, 0, sizeof(buffer_t));
	buffer->type = (cached == 1) ? BUFF_TYPE_IO_CACHED : BUFF_TYPE_IO_DIRECT;
	buffer->io_context = context;
	buffer->io_read    = read;
	buffer->io_write   = write;
} // }}}

/** @brief Associate buffer with read and write functions
 *  @param[in]  context  IO context for functions
 *  @param[in]  read     IO read function
 *  @param[in]  write    IO write function
 *  @param[in]  cached   Cache returned io buffers
 *  @return buffer_t *
 *  @return NULL
 */
buffer_t *      buffer_io_alloc            (void *context, func_buffer read, func_buffer write, int cached){ // {{{
	buffer_t *buffer = (buffer_t *)malloc(sizeof(buffer_t));
	buffer_io_init(buffer, context, read, write, cached);
	return buffer;
} // }}}

/** @brief Associate raw memory with buffer
 *  @param buffer Place to write buffer structure
 *  @param ptr    Pointer to raw memory
 *  @param size   Length of memory
 */
void            buffer_init_from_bare      (buffer_t *buffer, void *ptr, size_t size){ // {{{
	buffer_init(buffer);
	buffer_add_head_raw(buffer, ptr, size);
} // }}}
/** @brief Allocate new buffer associated with raw memory
 *  @param ptr    Pointer to raw memory
 *  @param size   Length of memory
 */
buffer_t *      buffer_alloc_from_bare     (void *ptr, size_t size){ // {{{
	buffer_t *buffer = (buffer_t *)malloc(sizeof(buffer_t));
	buffer_init_from_bare(buffer, ptr, size);
	return buffer;
} // }}}

/** @brief Copy raw memory to new buffer
 *  @param buffer Place to write buffer structure
 *  @param ptr    Raw memory
 *  @param size   Length of memory
 */
void            buffer_init_from_copy      (buffer_t *buffer, void *ptr, size_t size){ // {{{
	void *chunk;

	buffer_init(buffer);
	chunk = buffer_add_head_chunk(buffer, size);
	memcpy(chunk, ptr, size);
} // }}}

/** @brief Copy raw memory to allocated buffer
 *  @param ptr    Raw memory
 *  @param size   Length of memory
 */
buffer_t *      buffer_alloc_from_copy     (void *ptr, size_t size){ // {{{
	buffer_t *buffer = (buffer_t *)malloc(sizeof(buffer_t));
	buffer_init_from_copy(buffer, ptr, size);
	return buffer;
} // }}}

/** @brief Destory buffer
 *  @param  buffer  Buffer to destory
 */
void            buffer_destroy             (buffer_t *buffer){ // {{{
	buffer_cleanup(buffer);
} // }}}

/** @brief Free allocated buffer
 *  @param  buffer  Buffer to free
 */
void            buffer_free                (buffer_t *buffer){ // {{{
	buffer_destroy(buffer);
	free(buffer);
} // }}}

/** @brief Defragment buffer to one chunk and return pointer to this chunk
 *  @param  buffer Buffer to defragment
 *  @return ptr to defragmented memory
 *  @return NULL if buffer was empty
 */
void *          buffer_defragment          (buffer_t *buffer){ // {{{
	if(buffer->head == buffer->tail)
		return chunk_get_ptr(buffer->head);
	
	void *chunk = chunk_data_alloc(buffer->size);
	buffer_read(buffer, 0, chunk, buffer->size);
	buffer_cleanup(buffer);
	buffer_add_tail_any(buffer, chunk);
	
	return chunk;
} // }}}

/** @brief Empty buffer
 *  @param buffer Buffer to empty
 */
void            buffer_cleanup             (buffer_t *buffer){ // {{{
	void *chunk = buffer->head;
	void *chunkn;
	switch(buffer->type){
		case BUFF_TYPE_CHUNKED:
		case BUFF_TYPE_IO_CACHED:
			chunk = buffer->head;
			while(chunk != NULL){
				chunkn = chunk_next(chunk);
				chunk_free(chunk);
				chunk = chunkn;
			}
			buffer_init(buffer);
			break;
		case BUFF_TYPE_IO_DIRECT:
			break;
	}
} // }}}

/** @brief Get buffer length
 *  @param buffer Buffer
 */
inline size_t   buffer_get_size            (buffer_t *buffer){ // {{{
	if(buffer->type == BUFF_TYPE_CHUNKED)
		return buffer->size;
	else
		return 0;
} // }}}

static void     buffer_add_head_any        (buffer_t *buffer, void *chunk){ // {{{
	size_t size = chunk_get_size(chunk);
	buffer->size += size;

	if(buffer->head == NULL){
		buffer->tail = chunk;
	}else{
		((chunk_t *)chunk - 1)->next = buffer->head;
	}
	buffer->head = chunk;
} // }}}
static void     buffer_add_tail_any        (buffer_t *buffer, void *chunk){ // {{{
	size_t size = chunk_get_size(chunk);
	buffer->size += size;

	if(buffer->tail == NULL){
		buffer->head = chunk;
	}else{
		((chunk_t *)buffer->tail - 1)->next = chunk;
	}
	buffer->tail = chunk;
} // }}}

void *          buffer_add_head_chunk      (buffer_t *buffer, size_t size){ // {{{
	void *chunk = chunk_data_alloc(size);
	if(chunk == NULL) return NULL;
	
	buffer_add_head_any(buffer, chunk);
	return chunk;
} // }}}
void *          buffer_add_tail_chunk      (buffer_t *buffer, size_t size){ // {{{
	void *chunk = chunk_data_alloc(size);
	if(chunk == NULL) return NULL;
	
	buffer_add_tail_any(buffer, chunk);
	return chunk;
} // }}}
void            buffer_add_head_raw        (buffer_t *buffer, void *ptr, size_t size){ // {{{
	void *chunk = chunk_bare_alloc(ptr, size);
	if(chunk == NULL) return;
	
	buffer_add_head_any(buffer, chunk);
} // }}}
void            buffer_add_tail_raw        (buffer_t *buffer, void *ptr, size_t size){ // {{{
	void *chunk = chunk_bare_alloc(ptr, size);
	if(chunk == NULL) return;
	
	buffer_add_tail_any(buffer, chunk);
} // }}}

/** @brief Write to buffer
 *  @param[in] buffer        Buffer
 *  @param[in] write_offset  Offset to start writing
 *  @param[in] buf           Ptr to memory
 *  @param[in] buf_size      Write length
 *  @return Number of bytes written
 *  @return IO function error
 *  @return -1
 */
ssize_t         buffer_write               (buffer_t *buffer, off_t write_offset, void *buf, size_t buf_size){ // {{{
	size_t offset_diff;

	switch(buffer->type){
		case BUFF_TYPE_CHUNKED:
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
		case BUFF_TYPE_IO_DIRECT:
		case BUFF_TYPE_IO_CACHED:
			return buffer->io_write(buffer, write_offset, buf, buf_size);
	}
	return -1;
} // }}}

/** @brief Read from buffer
 *  @param[in]  buffer       Buffer
 *  @param[in]  read_offset  Offset to start writing
 *  @param[out] buf          Read to ptr
 *  @param[in]  buf_size     Read length
 *  @return Number of bytes read
 *  @return IO function error
 *  @return -1
 */
ssize_t         buffer_read                (buffer_t *buffer, off_t read_offset, void *buf, size_t buf_size){ // {{{
	ssize_t processed = 0;
	size_t  offset_diff;

	switch(buffer->type){
		case BUFF_TYPE_CHUNKED:
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
		case BUFF_TYPE_IO_DIRECT:
		case BUFF_TYPE_IO_CACHED:
			return buffer->io_read(buffer, read_offset, buf, buf_size);
	}
	return -1;
} // }}}

/** @brief Seek chunk in buffer
 *  @param[in]  buffer        Buffer to seek
 *  @param[in]  b_offset      Offset
 *  @param[in]  p_chunk       Found chunk pointer
 *  @param[in]  p_ptr         Memory ptr
 *  @param[in]  p_rest_size   Memory size
 *  @return 0 on success
 *  @return -EINVAL on error
 */
int      buffer_seek                (buffer_t *buffer, off_t b_offset, void **p_chunk, void **p_ptr, size_t *p_rest_size){ // {{{
	switch(buffer->type){
		case BUFF_TYPE_CHUNKED:
			if(b_offset == 0){
				if( (*p_chunk     = buffer->head) == NULL)
					return -1;
				*p_ptr       = chunk_get_ptr (buffer->head);
				*p_rest_size = chunk_get_size(buffer->head);
				return 0;
			}
			
			buffer_process(buffer, buffer_get_size(buffer), 0,
				do{
					if((offset + chunk_size) > b_offset){
						*p_chunk     = s_chunk;
						*p_ptr       = chunk      + (b_offset - offset);
						*p_rest_size = chunk_size - (b_offset - offset);
						return 0;
					}
				}while(0)
			);
			break;
		case BUFF_TYPE_IO_DIRECT:
			return -EINVAL;
		case BUFF_TYPE_IO_CACHED:
			// check cache
			return -EINVAL;
	};
	return -1;
} // }}}

/** @brief Compare two buffers
 *  @param[in]  buffer1      First buffer
 *  @param[in]  buffer1_off  First buffer offset
 *  @param[in]  buffer2      Second buffer
 *  @param[in]  buffer2_off  Second buffer offset
 *  @param[in]  size         Size to compare
 *  @return 0 if buffers equal
 *  @return 1 or -1 if buffers differs
 */
ssize_t         buffer_memcmp              (buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off, size_t size){ // {{{
	int     ret;
	size_t  chunk1_size, chunk2_size, cmp_size;
	void   *chunk1, *chunk2;
	void   *s_chunk1 = buffer1->head;
	void   *s_chunk2 = buffer2->head;
	char    chunk1_buf[MEMCMP_BUFF_SIZE];
	char    chunk2_buf[MEMCMP_BUFF_SIZE];
	
	chunk1_size = chunk2_size = 0;
	while(size > 0){
		if(chunk1_size == 0){
			switch(buffer_seek(buffer1, buffer1_off, &s_chunk1, &chunk1, &chunk1_size)){
				case -1:      // invalid seek offset
					return -EINVAL;
				case  0:      // seek ok
					break;
				case -EINVAL: // seeking io buffer, need manual read
					chunk1       = &chunk1_buf;
					chunk1_size  = buffer_read(buffer1, buffer1_off, &chunk1_buf, MEMCMP_BUFF_SIZE);
					break;
			};
			buffer1_off += chunk1_size;
		}
		if(chunk2_size == 0){
			switch(buffer_seek(buffer2, buffer2_off, &s_chunk2, &chunk2, &chunk2_size)){
				case -1:      // invalid seek offset
					return -EINVAL;
				case  0:      // seek ok
					break;
				case -EINVAL: // seeking io buffer, need manual read
					chunk2       = &chunk2_buf;
					chunk2_size  = buffer_read(buffer2, buffer2_off, &chunk2_buf, MEMCMP_BUFF_SIZE);
					break;
			};
			buffer2_off += chunk2_size;
		}
		
		cmp_size = (chunk1_size < chunk2_size) ? chunk1_size : chunk2_size;
		cmp_size = (cmp_size    < size)        ? cmp_size    : size;
		
		ret = memcmp(chunk1, chunk2, cmp_size);
		if(ret != 0)
			return ret;
		
		chunk1      += cmp_size;
		chunk2      += cmp_size;
		chunk1_size -= cmp_size;
		chunk2_size -= cmp_size;
		size        -= cmp_size;
	}
	return 0;
} // }}}

/** @brief Copy from one buffer to another
 *  @param[in]  buffer1      First buffer
 *  @param[in]  buffer1_off  First buffer offset
 *  @param[in]  buffer2      Second buffer
 *  @param[in]  buffer2_off  Second buffer offset
 *  @param[in]  size         Size to copy
 *  @return 0 on success
 *  @return -EINVAL on error
 */
ssize_t         buffer_memcpy              (buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off, size_t size){ // {{{
	size_t  chunk2_size, cpy_size;
	void   *chunk2;
	void   *s_chunk2 = buffer2->head;
	char    chunk2_buf[MEMCMP_BUFF_SIZE];
	
	chunk2_size = 0;
	while(size > 0){
		if(chunk2_size == 0){
			switch(buffer_seek(buffer2, buffer2_off, &s_chunk2, &chunk2, &chunk2_size)){
				case -1:      // invalid seek offset
					return -EINVAL;
				case  0:      // seek ok
					break;
				case -EINVAL: // seeking io buffer, need manual read
					chunk2       = &chunk2_buf;
					chunk2_size  = buffer_read(buffer2, buffer2_off, &chunk2_buf, MEMCMP_BUFF_SIZE);
					break;
			};
			buffer2_off += chunk2_size;
		}
		
		cpy_size = (chunk2_size < size) ? chunk2_size : size;
		
		buffer_write(buffer1, buffer1_off, chunk2, cpy_size);
		
		buffer1_off += cpy_size;
		chunk2      += cpy_size;
		chunk2_size -= cpy_size;
		size        -= cpy_size;
	}
	return 0;
} // }}}

ssize_t         buffer_strcmp              (buffer_t *buffer1, buffer_t *buffer_2);
size_t          buffer_strlen              (buffer_t *buffer);

/*
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

}}} */
/* }}}1 */
