#ifndef STRUCTS_BUFFER_H
#define STRUCTS_BUFFER_H

int           struct_get_member_value      (buffer_t *buffer, unsigned int member_id, subchunk_t *chunk);
_inline void  struct_assign                (buffer_t *buffer, struct_t *structure)                        { buffer->structure = structure; }

#endif // STRUCTS_BUFFER_H
