#ifndef DATA_H
#define DATA_H

typedef size_t (*f_data_len) (void *, size_t);
typedef int    (*f_data_cmp) (void *, void *);
typedef int    (*f_data_inc) (void *);
typedef int    (*f_data_div) (void *, unsigned int);
typedef int    (*f_data_mul) (void *, unsigned int);

typedef enum size_type {
	SIZE_FIXED,
	SIZE_VARIABLE
} size_type;

typedef struct data_proto_t {
	char *      type_str;
	data_type   type;
	size_type   size_type;
	
	f_data_cmp  func_cmp;
	f_data_len  func_len;
	f_data_div  func_div;
	f_data_mul  func_mul;
	f_data_inc  func_inc;
	
	size_t      fixed_size;
} data_proto_t;

typedef void   data_t;

extern data_proto_t  data_protos[];
extern size_t        data_protos_size;

/* api's */
//int                  data_proto_init              (data_proto_t *proto, data_type type);
int                  data_type_is_valid     (data_type type);
int                  data_cmp               (data_type type, data_t *data1, data_t *data2);
size_t               data_len               (data_type type, data_t *data, size_t buffer_size);
size_type            data_size_type         (data_type type);
//_inline f_data_cmp   data_proto_get_cmp_func      (data_proto_t *data)                   { return data->func_cmp; }
//_inline f_data_len   data_proto_get_len_func      (data_proto_t *data)                   { return data->func_len; }

data_type            data_type_from_string  (char *string);

#define REGISTER_DATA(type_name,size_type,func,...)

#endif // DATA_H
