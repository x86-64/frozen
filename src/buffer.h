#ifndef BUFFER_H
#define BUFFER_H

typedef struct buffer_t buffer_t;

typedef ssize_t (*func_buffer)(buffer_t *buffer, off_t offset, void *buf, size_t buf_size);


typedef enum buff_type {
	BUFF_TYPE_CHUNKED,
	BUFF_TYPE_IO_DIRECT,
	BUFF_TYPE_IO_CACHED
} buff_type;

typedef enum chunk_type {
	CHUNK_TYPE_ALLOCATED,
	CHUNK_TYPE_BARE
} chunk_type;

typedef struct chunk_t {
	chunk_type   type;
	size_t       size;
	void        *next;
	
	void        *bare_ptr; // only CHUNK_TYPE_BARE
} chunk_t;

struct buffer_t {
	buff_type    type;
	size_t       size;
	void        *head;
	void        *tail;
	
	void        *io_context;
	func_buffer  io_read;
	func_buffer  io_write;
};

/*
void            buffer_init                (buffer_t *buffer);
void            buffer_remove_chunks       (buffer_t *buffer);
void            buffer_destroy             (buffer_t *buffer);

buffer_t *      buffer_alloc               (void);
buffer_t *      buffer_from_data           (void *data, size_t data_size);
void            buffer_free                (buffer_t *buffer);

void            buffer_add_head_chunk      (buffer_t *buffer, void *chunk);
void            buffer_add_tail_chunk      (buffer_t *buffer, void *chunk);
void *          buffer_add_new_head_chunk  (buffer_t *buffer, size_t size);
void *          buffer_add_new_tail_chunk  (buffer_t *buffer, size_t size);
int             buffer_delete_chunk        (buffer_t *buffer, void *chunk);
void *          buffer_seek                (buffer_t *buffer, off_t ptr);

ssize_t         buffer_write               (buffer_t *buffer, off_t write_offset, void *buf, size_t buf_size);
ssize_t         buffer_read                (buffer_t *buffer, off_t read_offset,  void *buf, size_t buf_size);

_inline size_t  buffer_get_size            (buffer_t *buffer);
_inline void *  buffer_get_first_chunk     (buffer_t *buffer);
*/

void *          chunk_alloc                (size_t size);
_inline size_t  chunk_get_size             (void *chunk);
_inline void *  chunk_get_ptr              (void *chunk);
_inline void *  chunk_next                 (void *chunk);
_inline void    chunk_free                 (void *chunk);


void            buffer_init                (buffer_t *buffer);
buffer_t *      buffer_alloc               (void);

void            buffer_io_init             (buffer_t *buffer, void *context, func_buffer read, func_buffer write, int cached);
buffer_t *      buffer_io_alloc            (void *context, func_buffer read, func_buffer write, int cached);

void            buffer_init_from_bare      (buffer_t *buffer, void *ptr, size_t size);
buffer_t *      buffer_alloc_from_bare     (void *ptr, size_t size);
void            buffer_init_from_copy      (buffer_t *buffer, void *ptr, size_t size);
buffer_t *      buffer_alloc_from_copy     (void *ptr, size_t size);

void            buffer_destroy             (buffer_t *buffer);
void            buffer_free                (buffer_t *buffer);


void            buffer_defragment          (buffer_t *buffer);
void            buffer_cleanup             (buffer_t *buffer);
_inline size_t  buffer_get_size            (buffer_t *buffer);

ssize_t         buffer_write               (buffer_t *buffer, off_t offset, void *buf, size_t buf_size);
ssize_t         buffer_read                (buffer_t *buffer, off_t offset, void *buf, size_t buf_size);

void *          buffer_add_head_chunk      (buffer_t *buffer, size_t size);
void *          buffer_add_tail_chunk      (buffer_t *buffer, size_t size);
void            buffer_add_head_raw        (buffer_t *buffer, void *ptr, size_t size);
void            buffer_add_tail_raw        (buffer_t *buffer, void *ptr, size_t size);

ssize_t         buffer_memcmp              (buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off, size_t size);
ssize_t         buffer_strcmp              (buffer_t *buffer1, buffer_t *buffer_2);
size_t          buffer_strlen              (buffer_t *buffer);

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

