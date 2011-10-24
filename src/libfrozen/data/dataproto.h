#ifndef DATA_PROTO_H
#define DATA_PROTO_H

typedef enum data_api_type {
	API_DEFAULT_HANDLER,
	API_HANDLERS
} data_api_type;

typedef ssize_t    (*f_data_func)  (data_t *, void *args);

typedef struct data_proto_t {
	char                  *type_str;
	datatype_t             type;
	data_api_type          api_type;
	
	f_data_func            handler_default;
	f_data_func            handlers[ACTION_INVALID];
} data_proto_t;

extern data_proto_t *          data_protos[];
extern size_t                  data_protos_size;

#endif
