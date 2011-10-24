#ifndef DATA_DATAPTR_T_H
#define DATA_DATAPTR_T_H

#define DATA_DATAPTRT(_data)         { TYPE_DATAPTRT, _data }
#define DEREF_TYPE_DATAPTRT(_data) (data_t *)((_data)->ptr)
#define REF_TYPE_DATAPTRT(_dt) _dt
#define HAVEBUFF_TYPE_DATAPTRT 0

#endif
