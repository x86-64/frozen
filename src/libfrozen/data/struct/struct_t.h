#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

#define DATA_STRUCTT(_struct,_buffer) {TYPE_STRUCTT, NULL}

typedef hash_t  struct_t;
	
API size_t    struct_pack         (struct_t *structure, request_t *values, data_t *buffer);
API size_t    struct_unpack       (struct_t *structure, request_t *values, data_t *buffer);
API size_t    struct_unpack_copy  (struct_t *structure, request_t *values, data_t *buffer);

extern data_proto_t struct_t_proto;

#endif
