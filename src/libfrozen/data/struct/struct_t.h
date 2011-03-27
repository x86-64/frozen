#ifndef DATA_STRUCT_H
#define DATA_STRUCT_H

enum { TYPE_STRUCTT = 2 };
#define DATA_STRUCTT(_struct,_buffer) {TYPE_STRUCTT, NULL, 0}

typedef hash_t  struct_t;
	
API size_t    struct_pack      (struct_t *structure, request_t *values, data_t *buffer, data_ctx_t *ctx);
API size_t    struct_unpack    (struct_t *structure, request_t *values, data_t *buffer, data_ctx_t *ctx);

extern data_proto_t struct_t_proto;

#endif
