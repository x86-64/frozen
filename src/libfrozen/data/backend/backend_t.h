#ifndef DATA_BACKEND_H
#define DATA_BACKEND_H

enum { TYPE_BACKENDT = 1 };

#define DATA_BACKENDT(_backend) { TYPE_BACKENDT, (backend_t *[]){ _backend }, 0 }

extern data_proto_t backend_proto;

#endif
