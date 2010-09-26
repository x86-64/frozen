#ifndef STRUCTS_H
#define STRUCTS_H

typedef enum data_type {
	TYPE_INTEGER,
	TYPE_STRING_FIXED,
	TYPE_STRING_ZT
	// ...
} data_type;

typedef struct data_t {
	size_t     size;
	data_type  type;
} data_t;

typedef struct member_t {
	char *     name;
	data_t     type;
} member_t;

typedef struct struct_t {
	size_t    size;
	
} struct_t;


int       struct_get_size              (struct_t *structure);
int       struct_get_member_by_name    (struct_t *structure, char *name);


#endif // STRUCTS_H 
