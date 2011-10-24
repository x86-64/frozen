#ifndef DATA_H
#define DATA_H

#define DEF_BUFFER_SIZE 1024

typedef enum data_formats {
	FORMAT_BINARY,
	FORMAT_HUMANREADABLE
} data_formats;

typedef struct data_t {
	uintmax_t              type; // datatype_t
	void                  *ptr;
} data_t;

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
	(void)_ret;                                                            \
}

#define data_convert(_ret, _type, _dst, _src) {                                          \
	data_t __data_dst = { _type, REF_##_type(_dst) };                                \
	fastcall_convert_from _r_convert = { { 3, ACTION_CONVERT_FROM }, _src };         \
	_ret = data_query(&__data_dst, &_r_convert);                                     \
	_dst = DEREF_##_type(&__data_dst);                                               \
	(void)_ret;                                                                      \
};

#endif // DATA_H
