#ifndef DATA_RAW_T_H
#define DATA_RAW_T_H

#define DATA_RAW(_buf,_size) {TYPE_RAWT, (raw_t []){ { _buf, _size } }}
#define DEREF_TYPE_RAWT(_data) (raw_t *)((_data)->ptr)
#define REF_TYPE_RAWT(_dt) _dt
#define HAVEBUFF_TYPE_RAWT 0

typedef struct raw_t {
	void                  *ptr;
	uintmax_t              size;
} raw_t;

extern data_proto_t raw_t_proto;

#endif
