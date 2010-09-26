#ifndef DATA_H
#define DATA_H

typedef int (*f_data_cmp) (void *, void *);

typedef enum size_type {
	SIZE_FIXED,
	SIZE_VARIABLE
} size_type;

typedef struct data_t {
	char *      type_str;
	data_type   type;
	size_type   size_type;
	f_data_cmp  func_cmp;
	size_t      size;
} data_t;

extern data_t data_types[];
extern size_t data_types_size;

/* api's */
void         data_init              (data_t *data, data_type type);
f_data_cmp   data_get_cmp_func      (data_t *data);
data_type    data_type_from_string  (char *string);

#define REGISTER_DATA(type_name,size_type,func,...)

#endif // DATA_H
