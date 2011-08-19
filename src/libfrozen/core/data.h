#ifndef DATA_H
#define DATA_H

enum size_type {
	SIZE_FIXED,
	SIZE_VARIABLE
};

typedef void    (*f_data_alloc)     (data_t *, size_t);
typedef void    (*f_data_free)      (data_t *);

typedef size_t  (*f_data_len)       (data_t *, data_ctx_t *);
typedef size_t  (*f_data_len2raw)   (size_t);
typedef size_t  (*f_data_rawlen)    (data_t *, data_ctx_t *);

typedef int     (*f_data_cmp)       (data_t *, data_ctx_t *, data_t *, data_ctx_t *);
typedef int     (*f_data_arith)     (char, data_t *, data_ctx_t *, data_t *, data_ctx_t *);

typedef ssize_t (*f_data_read)      (data_t *, data_ctx_t *, off_t, void **, size_t *);
typedef ssize_t (*f_data_write)     (data_t *, data_ctx_t *, off_t, void *, size_t);
typedef ssize_t (*f_data_convert)   (data_t *, data_ctx_t *, data_t *, data_ctx_t *);
typedef ssize_t (*f_data_copy)      (data_t *, data_t *);

#define DATA_INVALID  { TYPE_INVALID, NULL, 0 }
#define DEF_BUFFER_SIZE 1024

struct data_t {
	data_type       type;
	void           *data_ptr;
	size_t          data_size;
};

struct data_proto_t {
	char *          type_str;
	data_type       type;
	size_type       size_type;
	size_t          fixed_size; // TODO IMPORTANT deprecate this
	
	f_data_alloc    func_alloc;
	f_data_free     func_free;
	
	f_data_len      func_len;       // return length of data in units
	f_data_len2raw  func_len2raw;   // return raw length needed to store n-unit data
	f_data_rawlen   func_rawlen;    // return raw length of data in bytes
	
	f_data_cmp      func_cmp;
	f_data_arith    func_arithmetic;
	
	f_data_read     func_read;
	f_data_write    func_write;
	f_data_convert  func_convert;
	f_data_copy     func_copy;
};

/* api's */

API ssize_t              data_type_validate     (data_type type);
API data_type            data_type_from_string  (char *string);
API char *               data_string_from_type  (data_type type);
API data_proto_t *       data_proto_from_type   (data_type type);

API size_t               data_len               (data_t *data, data_ctx_t *data_ctx);
API size_t               data_rawlen            (data_t *data, data_ctx_t *data_ctx);
API size_t               data_len2raw           (data_type type, size_t unitsize);

API int                  data_cmp               (data_t *data1, data_ctx_t *data1_ctx, data_t *data2, data_ctx_t *data2_ctx);
API int                  data_arithmetic        (char operator, data_t *operand1, data_ctx_t *operand1_ctx, data_t *operand2, data_ctx_t *operand2_ctx);

API ssize_t              data_convert           (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx);
API ssize_t              data_transfer          (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx);

API ssize_t              data_alloc             (data_t *dst, data_type type, size_t size);
API ssize_t              data_copy              (data_t *dst, data_t *src);
API ssize_t              data_read              (data_t *src, data_ctx_t *src_ctx, off_t offset, void *buffer, size_t size);
API ssize_t              data_write             (data_t *dst, data_ctx_t *dst_ctx, off_t offset, void *buffer, size_t size);
API ssize_t              data_validate          (data_t *data);
API void                 data_free              (data_t *data);

_inline
API data_type            data_value_type        (data_t *data);
_inline
API void *               data_value_ptr         (data_t *data); // TODO deprecate this
_inline
API size_t               data_value_len         (data_t *data); // TODO deprecate this

#define data_alloc_local(_dst,_type,_size) {                       \
	(_dst)->type      = _type;                                 \
	(_dst)->data_size = _size;                                 \
	(_dst)->data_ptr  = ((_size) != 0) ? alloca(_size) : NULL; \
}

#define data_copy_local(_dst,_src) {                                     \
	data_alloc_local(_dst, (_src)->type, (_src)->data_size);         \
	memcpy((_dst)->data_ptr, (_src)->data_ptr, (_src)->data_size);   \
}

#define data_assign_data_t(_dst,_src){           \
	memcpy((_dst), (_src), sizeof(data_t));  \
}

#define data_assign_dt(_dst,_type,_dt){          \
	(_dst)->type      = _type;               \
	(_dst)->data_ptr  = &(_dt);              \
	(_dst)->data_size = sizeof(_dt);         \
}

#define data_assign_raw(_dst,_type,_ptr,_size){  \
	(_dst)->type      = _type;               \
	(_dst)->data_size = _size;               \
	(_dst)->data_ptr  = _ptr;                \
}

#define data_convert_to_alloc(_retval,_type,_dst,_src,_src_ctx){ \
	size_t m_len = data_len(_src,_src_ctx);                  \
	m_len = data_len2raw(_type, m_len);                      \
	data_alloc(_dst,_type,m_len);                            \
	_retval = data_convert(_dst,NULL,_src,_src_ctx);         \
}

#define data_convert_to_local(_retval,_type,_dst,_src,_src_ctx){ \
	size_t m_len = data_len(_src,_src_ctx);                  \
	m_len = data_len2raw(_type, m_len);                      \
	data_alloc_local(_dst,_type,m_len);                      \
	_retval = data_convert(_dst,NULL,_src,_src_ctx);         \
}

#define data_convert_to_dt(_retval,_type,_dt,_src,_src_ctx){      \
	data_t m_dst;                                             \
	data_assign_dt(&m_dst,_type,_dt);                         \
	_retval = data_convert(&m_dst,NULL,_src,_src_ctx);        \
}

#define data_to_dt(_retval,_type,_dt,_src,_src_ctx){                 \
	if((_src)->type == _type){                                   \
		_dt = GET_##_type(_src);                             \
		_retval = 0;                                         \
	}else{                                                       \
		data_convert_to_dt(_retval,_type,_dt,_src,_src_ctx); \
	}                                                            \
}

#endif // DATA_H

/* vim: set filetype=c: */
