#ifndef DATA_RAW_T_H
#define DATA_RAW_T_H

#define DATA_RAW(_buf,_size) {TYPE_RAWT, (raw_t []){ { _buf, _size } }}

typedef struct raw_t {
	void                  *ptr;
	uintmax_t              size;
} raw_t;

extern data_proto_t raw_t_proto;

#endif
