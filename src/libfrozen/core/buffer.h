/**
 * @file buffer.h
 * @ingroup buffer
 * @brief Buffers header
 */

#ifndef BUFFER_H
#define BUFFER_H

typedef struct buffer_t buffer_t;

typedef ssize_t (*func_buffer)(buffer_t *buffer, off_t offset, void *buf, size_t buf_size);

/// @brief Buffer types
typedef enum buff_type {
	BUFF_TYPE_CHUNKED,
	BUFF_TYPE_IO_DIRECT,
	BUFF_TYPE_IO_CACHED
} buff_type;

/// @brief Chunk types
typedef enum chunk_type {
	CHUNK_TYPE_ALLOCATED,
	CHUNK_TYPE_BARE
} chunk_type;

/// @brief chunk_t structure
typedef struct chunk_t {
	chunk_type   type;       ///< Chunk type
	size_t       size;       ///< Chunk size
	void        *cnext;      ///< Next chunk ptr
	
	void        *bare_ptr;   ///< Ptr to raw memory (only CHUNK_TYPE_BARE)
} chunk_t;

/// @brief buffer_t structure
struct buffer_t {
	buff_type    type;       ///< Buffer type
	size_t       size;       ///< Buffer length
	void        *head;       ///< First buffer chunk
	void        *tail;       ///< Last buffer chunk
	
	void        *io_context; ///< IO context
	func_buffer  io_read;    ///< IO read function
	func_buffer  io_write;   ///< IO write function
};

void *          chunk_alloc                (size_t size);
_inline size_t  chunk_get_size             (void *chunk);
_inline void *  chunk_get_ptr              (void *chunk);
_inline void *  chunk_next                 (void *chunk);
_inline void    chunk_free                 (void *chunk);


API void            buffer_init                (buffer_t *buffer);
API buffer_t *      buffer_alloc               (void);

API void            buffer_io_init             (buffer_t *buffer, void *context, func_buffer read, func_buffer write, int cached);
API buffer_t *      buffer_io_alloc            (void *context, func_buffer read, func_buffer write, int cached);

API void            buffer_init_from_bare      (buffer_t *buffer, void *ptr, size_t size);
API buffer_t *      buffer_alloc_from_bare     (void *ptr, size_t size);
API void            buffer_init_from_copy      (buffer_t *buffer, void *ptr, size_t size);
API buffer_t *      buffer_alloc_from_copy     (void *ptr, size_t size);

API void            buffer_destroy             (buffer_t *buffer);
API void            buffer_free                (buffer_t *buffer);


API void *          buffer_defragment          (buffer_t *buffer);
API void            buffer_cleanup             (buffer_t *buffer);
API _inline size_t  buffer_get_size            (buffer_t *buffer);
API int             buffer_seek                (buffer_t *buffer, off_t b_offset, void **p_chunk, void **p_ptr, size_t *p_rest_size);

API ssize_t         buffer_write               (buffer_t *buffer, off_t offset, void *buf, size_t buf_size);
API ssize_t         buffer_read                (buffer_t *buffer, off_t offset, void *buf, size_t buf_size);
// TODO bad read\write api

API void *          buffer_add_head_chunk      (buffer_t *buffer, size_t size);
API void *          buffer_add_tail_chunk      (buffer_t *buffer, size_t size);
API void            buffer_add_head_raw        (buffer_t *buffer, void *ptr, size_t size);
API void            buffer_add_tail_raw        (buffer_t *buffer, void *ptr, size_t size);

API ssize_t         buffer_memcpy              (buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off, size_t size);
API ssize_t         buffer_memcmp              (buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off, size_t size);
API ssize_t         buffer_strcmp              (buffer_t *buffer1, buffer_t *buffer_2);
API size_t          buffer_strlen              (buffer_t *buffer);

#define buffer_process(_buffer,_size,_create,_func)  ({                     \
	off_t  offset;                                                      \
	size_t chunk_size, to_process, size;                                \
	void * s_chunk = _buffer->head;                                     \
	void * chunk;                                                       \
	                                                                    \
	to_process = _size;                                                 \
	offset     = 0;                                                     \
		                                                            \
	while(to_process > 0){                                              \
		if(s_chunk == NULL){                                        \
			if(_create == 1){                                   \
				s_chunk = buffer_add_tail_chunk(            \
					_buffer,                            \
					to_process                          \
				);                                          \
			}else{                                              \
				break;                                      \
			}                                                   \
		}                                                           \
		chunk_size = chunk_get_size(s_chunk);                       \
		chunk      = chunk_get_ptr (s_chunk);                       \
	 	                                                            \
		size = (chunk_size > to_process) ? to_process : chunk_size; \
		_func;                                                      \
		                                                            \
		offset     += size;                                         \
		to_process -= size;                                         \
		s_chunk     = chunk_next(s_chunk);                          \
	}                                                                   \
})

#endif // BUFFER_H

