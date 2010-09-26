#ifndef BUFFER_H
#define BUFFER_H

typedef struct chunk_t {
	size_t  size;
	void   *next;
} chunk_t;

typedef struct subchunk_t {
	size_t  size;
	void   *ptr;
} subchunk_t;

void *         chunk_alloc    (size_t size);
_inline size_t chunk_get_size (void *chunk){ return ( ((chunk_t *)chunk) - 1)->size; }
_inline void * chunk_next     (void *chunk){ return ( ((chunk_t *)chunk) - 1)->next; }
_inline void   chunk_free     (void *chunk){ free( ((chunk_t *)chunk) - 1); }


typedef struct buffer_t {
	size_t      size;
	void       *head;
	void       *tail;
} buffer_t;

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
ssize_t         buffer_read                (buffer_t *buffer, off_t read_offset, void *buf, size_t buf_size);

_inline size_t  buffer_get_size            (buffer_t *buffer)               { return buffer->size; }
_inline void *  buffer_get_first_chunk     (buffer_t *buffer)               { return buffer->head; }

#define buffer_process(_buffer,_size,_create,_func)  ({                     \
	off_t  offset;                                                      \
	size_t chunk_size, to_process, size;                                \
	void * chunk = buffer_get_first_chunk(_buffer);                     \
	                                                                    \
	to_process = _size;                                                 \
	offset     = 0;                                                     \
		                                                            \
	while(to_process > 0){                                              \
		if(chunk == NULL){                                          \
			if(_create == 1){                                   \
				chunk = buffer_add_new_tail_chunk(          \
					_buffer,                            \
					to_process                          \
				);                                          \
			}else{                                              \
				break;                                      \
			}                                                   \
		}                                                           \
		chunk_size = chunk_get_size(chunk);                         \
		                                                            \
		size = (chunk_size > to_process) ? to_process : chunk_size; \
		_func;                                                      \
		                                                            \
		offset     += size;                                         \
		to_process -= size;                                         \
		chunk       = chunk_next(chunk);                            \
	}                                                                   \
})

#endif // BUFFER_H
