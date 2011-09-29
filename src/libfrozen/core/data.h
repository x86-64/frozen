#ifndef DATA_H
#define DATA_H

#define DATA_INVALID  { TYPE_INVALID, NULL, 0 }
#define DEF_BUFFER_SIZE 1024

typedef enum data_functions {
	ACTION_ALLOC,
	ACTION_FREE,
	ACTION_PHYSICALLEN,
	ACTION_LOGICALLEN,
	ACTION_CONVERTLEN,
	ACTION_COMPARE,
	ACTION_INCREMENT,
	ACTION_DECREMENT,
	ACTION_ADD,
	ACTION_SUB,
	ACTION_MULTIPLY,
	ACTION_DIVIDE,
	ACTION_READ,
	ACTION_WRITE,
	ACTION_CONVERT,
	ACTION_TRANSFER,
	ACTION_COPY,
	
	ACTION_GETDATAPTR,

	ACTION_LAST
} data_functions;

typedef enum data_api_type {
	API_DEFAULT_HANDLER,
	API_HANDLERS
} data_api_type;

struct data_t {
	data_type       type;
	void           *ptr;
};

typedef ssize_t    (*f_data_func)  (data_t *, void *args);

struct data_proto_t {
	char *          type_str;
	data_type       type;
	data_api_type   api_type;
	
	f_data_func     handler_default;
	f_data_func     handlers[ACTION_LAST];
};


typedef struct fastcall_header {
	uintmax_t              nargs;
	uintmax_t              action;
} fastcall_header;

typedef struct fastcall_io {
	fastcall_header        header;
	uintmax_t              offset;
	void                  *buffer;
	uintmax_t              buffer_size;
} fastcall_io;
typedef struct fastcall_io     fastcall_read;
typedef struct fastcall_io     fastcall_write;

typedef struct fastcall_len {
	fastcall_header        header;
	uintmax_t              length;
} fastcall_len;
typedef struct fastcall_len    fastcall_physicallen;
typedef struct fastcall_len    fastcall_logicallen;

typedef struct fastcall_alloc {
	fastcall_header        header;
	uintmax_t              length;
} fastcall_alloc;

typedef struct fastcall_free {
	fastcall_header        header;
} fastcall_free;

typedef struct fastcall_copy {
	fastcall_header        header;
	data_t                *dest;
} fastcall_copy;

typedef struct fastcall_transfer {
	fastcall_header        header;
	data_t                *dest;
} fastcall_transfer;

typedef struct fastcall_convert {
	fastcall_header        header;
	data_t                *src;
} fastcall_convert;

typedef struct fastcall_compare {
	fastcall_header        header;
	data_t                *data2;
} fastcall_compare;

typedef struct fastcall_arith {
	fastcall_header        header;
	data_t                *data2;
} fastcall_arith;
typedef struct fastcall_arith  fastcall_add;
typedef struct fastcall_arith  fastcall_sub;
typedef struct fastcall_arith  fastcall_mul;
typedef struct fastcall_arith  fastcall_div;

typedef struct fastcall_arith_no_arg {
	fastcall_header        header;
} fastcall_arith_no_arg;
typedef struct fastcall_arith_no_arg fastcall_increment;
typedef struct fastcall_arith_no_arg fastcall_decrement;

typedef struct fastcall_getdataptr {
	fastcall_header        header;
	void                  *ptr;
} fastcall_getdataptr;

#ifdef DATA_C
uintmax_t fastcall_nargs[ACTION_LAST] = {
	[ACTION_READ] = 5,
	[ACTION_WRITE] = 5,
	[ACTION_PHYSICALLEN] = 3,
	[ACTION_LOGICALLEN] = 3,
	[ACTION_COPY] = 3,
	[ACTION_CONVERT] = 3,
	[ACTION_COMPARE] = 3,
	[ACTION_ALLOC] = 3,
	[ACTION_FREE] = 2,
	[ACTION_ADD] = 3,
	[ACTION_SUB] = 3,
	[ACTION_MULTIPLY] = 3,
	[ACTION_DIVIDE] = 3,
	[ACTION_INCREMENT] = 2,
	[ACTION_DECREMENT] = 2,
	[ACTION_TRANSFER]  = 3,
	[ACTION_GETDATAPTR] = 3,
};
#endif

extern data_proto_t * data_protos[];
extern size_t         data_protos_size;

/* api's */

API ssize_t              data_type_validate     (data_type type);
API data_type            data_type_from_string  (char *string);
API char *               data_string_from_type  (data_type type);
API data_proto_t *       data_proto_from_type   (data_type type);

API ssize_t              data_query             (data_t *data, void *args);

#define data_get(_ret,_type,_dt,_src){                                         \
	if((_src) != NULL && (_src)->type == _type){                           \
		_dt  = DEREF_##_type(_src);                                    \
		_ret = 0;                                                      \
	}else{                                                                 \
		if( HAVEBUFF_##_type == 1 ){                                   \
			data_convert(_ret, _type, _dt, _src);                  \
		}else{                                                         \
			_ret = -EINVAL;                                        \
		}                                                              \
	}                                                                      \
}

#define data_convert(_ret, _type, _dst, _src) {                                \
	data_t __data_dst = { _type, REF_##_type(_dst) };                      \
	fastcall_convert _r_convert = { { 3, ACTION_CONVERT }, _src };         \
	_ret = data_query(&__data_dst, &_r_convert);                           \
	_dst = DEREF_##_type(&__data_dst);                                     \
};

#endif // DATA_H

/* vim: set filetype=c: */
