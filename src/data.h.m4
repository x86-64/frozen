#ifndef DATA_H
#define DATA_H

/* m4 {{{
m4_define(`REGISTER_TYPE', `
        m4_define(`TYPES_ARRAY', m4_defn(`TYPES_ARRAY')`$1,
')')
m4_define(`REGISTER_MACRO', `
	m4_define(`MACRO_ARRAY',
		m4_defn(`MACRO_ARRAY')
		`#define $1 $2'
	)
')
m4_divert(-1)
m4_include(data_protos.m4)
m4_divert(0)
}}} */

MACRO_ARRAY()

typedef enum data_type { TYPES_ARRAY() } data_type;

typedef void                   data_t;
typedef struct data_proto_t    data_proto_t;
typedef struct data_ctx_t      data_ctx_t;

typedef void * (*f_data_ctx_n)   (void *);
typedef void   (*f_data_ctx_f)   (void *);

typedef size_t (*f_data_len_b)   (data_ctx_t *, buffer_t *, off_t);
typedef int    (*f_data_cmp_b)   (data_ctx_t *, buffer_t *, off_t, buffer_t *, off_t);
typedef int    (*f_data_arith_b) (data_ctx_t *, buffer_t *, off_t, char, unsigned int);

typedef size_t (*f_data_len_r)   (void *, size_t);
typedef int    (*f_data_cmp_r)   (void *, void *);
typedef int    (*f_data_arith_r) (void *, char, unsigned int);

typedef enum size_type {
	SIZE_FIXED,
	SIZE_VARIABLE
} size_type;

struct data_ctx_t {
	data_proto_t *data_proto;
	void         *user_data;
};

struct data_proto_t {
	char *          type_str;
	data_type       type;
	size_type       size_type;
	
	f_data_ctx_n    func_ctx_parse;
	f_data_ctx_f    func_ctx_free;
	
	f_data_len_b    func_buffer_len;
	f_data_cmp_b    func_buffer_cmp;
	f_data_arith_b  func_buffer_arithmetic;
	
	f_data_len_r    func_bare_len;
	f_data_cmp_r    func_bare_cmp;
	f_data_arith_r  func_bare_arithmetic;
	
	size_t        fixed_size;
};

extern data_proto_t  data_protos[];
extern size_t        data_protos_size;

/* api's */
API data_proto_t *       data_proto_from_type   (data_type type);
API int                  data_type_is_valid     (data_type type);
API data_type            data_type_from_string  (char *string);

API int                  data_ctx_init          (data_ctx_t *ctx, data_type type, void *context);
API int                  data_ctx_destroy       (data_ctx_t *ctx);

API size_t               data_buffer_len        (data_ctx_t *ctx, buffer_t *buffer, off_t buffer_off);
API int                  data_buffer_cmp        (data_ctx_t *ctx, buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off);
API int                  data_buffer_arithmetic (data_ctx_t *ctx, buffer_t *buffer, off_t buffer_off, char operator, unsigned int operand);

API size_t               data_bare_len          (data_type type, data_t *data,  size_t buffer_size);
API int                  data_bare_cmp          (data_type type, data_t *data1, data_t *data2);
API int                  data_bare_arithmetic   (data_type type, data_t *data, char operator, unsigned int operand);

API size_type            data_size_type         (data_type type);

#endif // DATA_H

/* vim: set filetype=c: */
