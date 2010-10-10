#ifndef DATA_H
#define DATA_H

/* m4 {{{
m4_define(`REGISTER_TYPE', `
        m4_define(`TYPES_ARRAY', m4_defn(`TYPES_ARRAY')`$1,
')')
m4_divert(-1)
m4_include(data_protos.m4)
m4_divert(0)
}}} */

typedef enum data_type { TYPES_ARRAY() } data_type;

typedef void                   data_t;
typedef struct data_proto_t    data_proto_t;
typedef struct data_ctx_t      data_ctx_t;

typedef void * (*f_data_ctx_n) (void *);
typedef void   (*f_data_ctx_f) (void *);

typedef size_t (*f_data_len_b) (data_ctx_t *, buffer_t *, off_t);
typedef int    (*f_data_cmp_b) (data_ctx_t *, buffer_t *, off_t, buffer_t *, off_t);

typedef size_t (*f_data_len_r) (void *, size_t);
typedef int    (*f_data_cmp_r) (void *, void *);

typedef int    (*f_data_div)   (void *, unsigned int);
typedef int    (*f_data_mul)   (void *, unsigned int);
typedef int    (*f_data_add)   (void *, unsigned int);
typedef int    (*f_data_sub)   (void *, unsigned int);

typedef enum size_type {
	SIZE_FIXED,
	SIZE_VARIABLE
} size_type;

struct data_ctx_t {
	data_proto_t *data_proto;
	void         *user_data;
};

struct data_proto_t {
	char *        type_str;
	data_type     type;
	size_type     size_type;
	
	f_data_ctx_n  func_ctx_parse;
	f_data_ctx_f  func_ctx_free;
	
	f_data_len_b  func_buffer_len;
	f_data_cmp_b  func_buffer_cmp;
	
	f_data_len_r  func_bare_len;
	f_data_cmp_r  func_bare_cmp;
	
	f_data_div    func_div; // TODO remove it, replace with func_arithmetic
	f_data_mul    func_mul;
	f_data_add    func_add;
	f_data_sub    func_sub;
	
	size_t        fixed_size;
};

extern data_proto_t  data_protos[];
extern size_t        data_protos_size;

/* api's */
data_proto_t *       data_proto_from_type   (data_type type);
int                  data_type_is_valid     (data_type type);
data_type            data_type_from_string  (char *string);

int                  data_ctx_new           (data_ctx_t *ctx, data_type type, void *context);
int                  data_ctx_free          (data_ctx_t *ctx);

size_t               data_buffer_len        (data_ctx_t *ctx, buffer_t *buffer, off_t buffer_off);
int                  data_buffer_cmp        (data_ctx_t *ctx, buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off);

size_t               data_bare_len          (data_type type, data_t *data,  size_t buffer_size);
int                  data_bare_cmp          (data_type type, data_t *data1, data_t *data2);

size_type            data_size_type         (data_type type);

#endif // DATA_H

/* vim: set filetype=c: */
