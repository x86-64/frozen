#include <libfrozen.h>
#include <buffer_t.h>

/**
 * @file buffer.c
 * @ingroup buffer
 * @brief Working with buffer_t
 */

static ssize_t data_buffer_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = buffer_get_size( (buffer_t *)data->ptr );
	return 0;
} // }}}
static ssize_t data_buffer_t_read(data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret               = -1;
	uintmax_t              transfered        = 0;
	
	if(fargs->buffer == NULL)
		return -EINVAL;
	
	if(data->ptr == NULL){
		if( (ret = buffer_read( (buffer_t *)data->ptr, fargs->offset, fargs->buffer, fargs->buffer_size)) != -1)
			transfered = ret;
	}
	fargs->buffer_size = transfered;
	return ret;
} // }}}
static ssize_t data_buffer_t_write(data_t *data, fastcall_write *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->buffer == NULL)
		return -EINVAL;
	
	if(data->ptr == NULL)
		data->ptr = buffer_alloc();
	
	if( (ret = buffer_write( (buffer_t *)data->ptr, fargs->offset, fargs->buffer, fargs->buffer_size)) == -1){
		fargs->buffer_size = 0;
		return -1; // EOF
	}
	fargs->buffer_size = ret;
	return 0;
} // }}}

data_proto_t buffer_t_proto = {
	.type                   = TYPE_BUFFERT,
	.type_str               = "buffer_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_PHYSICALLEN] = (f_data_func)&data_buffer_t_len,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_buffer_t_len,
		[ACTION_READ]        = (f_data_func)&data_buffer_t_read,
		[ACTION_WRITE]       = (f_data_func)&data_buffer_t_write,
	}
};


static void     buffer_add_head_any        (buffer_t *buffer, void *chunk);
static void     buffer_add_tail_any        (buffer_t *buffer, void *chunk);

/* chunks - public {{{ */
void *      chunk_data_alloc (size_t size){ // {{{
	chunk_t *chunk;

	chunk = malloc(size + sizeof(chunk_t));
	chunk->type = CHUNK_TYPE_ALLOCATED;
	chunk->size = size;
	chunk->cnext = NULL;

	return (void *)(chunk + 1);
} // }}}
void *      chunk_bare_alloc (void *ptr, size_t size){ // {{{
	chunk_t *chunk;

	chunk = malloc(sizeof(chunk_t));
	chunk->type     = CHUNK_TYPE_BARE;
	chunk->bare_ptr = ptr;
	chunk->size     = size;
	chunk->cnext     = NULL;

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
inline void * chunk_next     (void *chunk){ return ( ((chunk_t *)chunk) - 1)->cnext; }
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

/** @brief Destroy buffer
 *  @param  buffer  Buffer to destroy
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
		((chunk_t *)chunk - 1)->cnext = buffer->head;
	}
	buffer->head = chunk;
} // }}}
static void     buffer_add_tail_any        (buffer_t *buffer, void *chunk){ // {{{
	size_t size = chunk_get_size(chunk);
	buffer->size += size;

	if(buffer->tail == NULL){
		buffer->head = chunk;
	}else{
		((chunk_t *)buffer->tail - 1)->cnext = chunk;
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
			return buf_size;
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
				if( (*p_chunk = buffer->head) == NULL)
					goto error;
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
error:
	return -1;
} // }}}


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
				( (chunk_t *)last - 1)->cnext = ( (chunk_t *)curr - 1)->cnext;
			if(buffer->tail == curr)
				buffer->tail = last;
			if(buffer->head == curr)
				buffer->head = ( (chunk_t *)curr - 1)->cnext;
			ret = 0;
			break;
		}
		last = curr;
		curr = ( (chunk_t *)curr - 1)->cnext;
	}
	if(ret == 0){
		size = chunk_get_size(chunk);
		buffer->size -= size;

		chunk_free(chunk);
	}
	return ret;
} */ 
/* }}}1 */
