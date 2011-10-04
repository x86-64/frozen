#ifndef DATA_BACKEND_H
#define DATA_BACKEND_H

#define DATA_BACKENDT(_backend) { TYPE_BACKENDT, _backend }
#define DEREF_TYPE_BACKENDT(_data) (backend_t *)((_data)->ptr)
#define REF_TYPE_BACKENDT(_dt) _dt
#define HAVEBUFF_TYPE_BACKENDT 1

extern data_proto_t backend_t_proto;

#endif
