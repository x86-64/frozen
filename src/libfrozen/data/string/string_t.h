#ifndef DATA_STRING_T_H
#define DATA_STRING_T_H

#define DATA_STRING(value)          { TYPE_STRINGT, value, sizeof(value) }
#define DATA_PTR_STRING(value,size) { TYPE_STRINGT, value, size }
#define DATA_PTR_STRING_AUTO(value) { TYPE_STRINGT, value, strlen(value)+1 }
#define DT_STRING                   char *
#define GET_TYPE_STRINGT(value)     ((char *)value->data_ptr)

extern data_proto_t string_t_proto;

#endif
