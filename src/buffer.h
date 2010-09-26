#ifndef BUFFER_H
#define BUFFER_H

typedef struct buffer_t {
	size_t  size;
	void   *head;
	void   *tail;
} buffer_t;

typedef struct chunk_t {
	size_t  size;
	void   *next;
} chunk_t;

void        buffer_init          (buffer_t *buffer);
void        buffer_remove_chunks (buffer_t *buffer);
void        buffer_destroy       (buffer_t *buffer);
buffer_t *  buffer_alloc         (void);
void        buffer_free          (buffer_t *buffer);

void        buffer_add_head_chunk  (buffer_t *buffer, void *chunk);
void        buffer_add_tail_chunk  (buffer_t *buffer, void *chunk);
int         buffer_delete_chunk    (buffer_t *buffer, void *chunk);
size_t      buffer_get_size        (buffer_t *buffer);
void *      buffer_get_first_chunk (buffer_t *buffer);

void *      chunk_alloc    (size_t size);
void        chunk_free     (void *chunk);
size_t      chunk_get_size (void *chunk);
void *      chunk_next     (void *chunk);

#endif // BUFFER_H
