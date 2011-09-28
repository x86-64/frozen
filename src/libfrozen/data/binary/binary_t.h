#ifndef DATA_BINARY_T_H
#define DATA_BINARY_T_H

extern data_proto_t binary_t_proto;
#define DEREF_TYPE_BINARYT(_data) (void *)((_data)->ptr)
#define REF_TYPE_BINARYT(_dt) _dt
#define HAVEBUFF_TYPE_BINARYT 0

#endif
