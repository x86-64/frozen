#ifndef DATA_BACKEND_H
#define DATA_BACKEND_H

#define DATA_BACKENDT(_backend) { TYPE_BACKENDT, (backend_t *[]){ _backend }, 0 }
#define GET_TYPE_BACKENDT(value)     ((backend_t *)value->data_ptr)

extern data_proto_t backend_t_proto;

#endif
