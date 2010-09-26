#ifndef DATA_H
#define DATA_H

typedef enum data_type {
	TYPE_INTEGER,
	TYPE_STRING_FIXED,
	TYPE_STRING_ZT
	// ...
} data_type;

typedef enum size_type {
	SIZE_FIXED,
	SIZE_VARIABLE
} size_type;



typedef struct data_t {
	data_type  type;
	size_type  size_type;
	size_t     size;
} data_t;


typedef int (*f_data_cmp) (void *, void *);


void         data_init              (data_t *data, data_type type);
f_data_cmp   data_get_cmp_func      (data_t *data);

#endif // DATA_H
