#ifndef DATA_STRING_T_H
#define DATA_STRING_T_H

#define DATA_STRING(value)          { TYPE_STRINGT, value }
#define DATA_PTR_STRING(value)      { TYPE_STRINGT, value }
#define DEREF_TYPE_STRINGT(_data) (char *)((_data)->ptr)
#define REF_TYPE_STRINGT(_dt) _dt
#define HAVEBUFF_TYPE_STRINGT 0

#endif
