#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#define DATA_STRUCTT(_struct,_buffer) {TYPE_STRUCTT, NULL}
#define DEREF_TYPE_STRUCTT(_data) (struct_t *)((_data)->ptr)
#define REF_TYPE_STRUCTT(_dt) _dt
#define HAVEBUFF_TYPE_STRUCTT 0

typedef hash_t  struct_t;
	
uintmax_t    struct_pack         (struct_t *structure, request_t *values, data_t *buffer);
uintmax_t    struct_unpack       (struct_t *structure, request_t *values, data_t *buffer);

#endif
