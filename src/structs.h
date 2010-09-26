#ifndef STRUCTS_H
#define STRUCTS_H

typedef struct member_t {
	member_t * next;
	
	char *     name;
	data_t     data;
} member_t;

typedef struct struct_t {
	size_t     size;
	member_t * members;
} struct_t;

struct_t *  struct_new                   (setting_t *config);
void        struct_free                  (struct_t *structure);

int         struct_get_size              (struct_t *structure);
int         struct_get_member_by_name    (struct_t *structure, char *name);


#endif // STRUCTS_H 
