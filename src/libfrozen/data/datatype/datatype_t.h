#ifndef DATA_DATATYPE_T_H
#define DATA_DATATYPE_T_H

#define DATA_DATATYPET(...)  { TYPE_DATATYPET, (datatype_t []){ __VA_ARGS__ } }
#define DEREF_TYPE_DATATYPET(_data) *(datatype_t *)((_data)->ptr)
#define REF_TYPE_DATATYPET(_dt) (&(_dt))
#define HAVEBUFF_TYPE_DATATYPET 1

#endif
