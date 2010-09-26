#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct member_t {
	char *     name;
	data_t     data;
} member_t;

typedef struct struct_t {
	size_t     size;
		
	size_t     members_count;
	member_t * members;
} struct_t;

struct_t *      struct_new                   (setting_t *config);
void            struct_free                  (struct_t *structure);

_inline size_t  struct_get_size              (struct_t *structure){ return structure->size; }
member_t *      struct_get_member_by_name    (struct_t *structure, char *name);

#endif // STRUCTS_H 
