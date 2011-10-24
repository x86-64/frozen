#ifndef DATA_VOID_T_H
#define DATA_VOID_T_H

#define DATA_VOID      { TYPE_VOIDT, NULL }
#define DEREF_TYPE_VOIDT(_data) (void *)((_data)->ptr)
#define REF_TYPE_VOIDT(_dt) _dt
#define HAVEBUFF_TYPE_VOIDT 0

#endif
