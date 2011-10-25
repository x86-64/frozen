#ifndef API_H
#define API_H

/** @file api.h */

typedef enum api_types {
	API_HASH = 1,
	API_CRWD = 2,
	API_FAST = 4
} api_types;

typedef ssize_t (*f_crwd)      (backend_t *, request_t *);
typedef ssize_t (*f_fast_func) (backend_t *, void *);

typedef enum data_functions {
	ACTION_CREATE,
	ACTION_DELETE,
	ACTION_MOVE,
	ACTION_COUNT,
	ACTION_CUSTOM,
	ACTION_REBUILD,

	ACTION_ALLOC,
	ACTION_FREE,
	ACTION_INIT,
	ACTION_PHYSICALLEN,
	ACTION_LOGICALLEN,
	ACTION_COMPARE,
	ACTION_INCREMENT,
	ACTION_DECREMENT,
	ACTION_ADD,
	ACTION_SUB,
	ACTION_MULTIPLY,
	ACTION_DIVIDE,
	ACTION_READ,
	ACTION_WRITE,
	ACTION_CONVERT_TO,
	ACTION_CONVERT_FROM,
	ACTION_TRANSFER,
	ACTION_COPY,
	ACTION_IS_NULL,

	ACTION_GETDATAPTR,

	ACTION_INVALID
} data_functions;

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

typedef struct fastcall_init {
	fastcall_header        header;
	char                  *string;
} fastcall_init;

typedef struct fastcall_copy {
	fastcall_header        header;
	data_t                *dest;
} fastcall_copy;

typedef struct fastcall_transfer {
	fastcall_header        header;
	data_t                *dest;
} fastcall_transfer;

struct fastcall_convert_from {
	fastcall_header        header;
	data_t                *src;
	uintmax_t              format;
};
struct fastcall_convert_to {
	fastcall_header        header;
	data_t                *dest;
	uintmax_t              format;
};
typedef struct fastcall_convert_to    fastcall_convert_to;
typedef struct fastcall_convert_from  fastcall_convert_from;

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

typedef struct fastcall_is_null {
	fastcall_header        header;
	uintmax_t              is_null;
} fastcall_is_null;

typedef struct fastcall_create {
	fastcall_header        header;
	uintmax_t              size;
	uintmax_t              offset;
} fastcall_create;

typedef struct fastcall_delete {
	fastcall_header        header;
	uintmax_t              offset;
	uintmax_t              size;
} fastcall_delete;

typedef struct fastcall_move {
	fastcall_header        header;
	uintmax_t              offset_from;
	uintmax_t              offset_to;
	uintmax_t              size;
} fastcall_move;

typedef struct fastcall_count {
	fastcall_header        header;
	uintmax_t              nelements;
} fastcall_count;


extern uintmax_t fastcall_nargs[];

#endif
