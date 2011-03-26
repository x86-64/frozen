#ifndef DATA_STRING_T_H
#define DATA_STRING_T_H


enum { TYPE_STRING = 16 };
#define DATA_STRING(value)          { TYPE_STRING, value, sizeof(value) }
#define DATA_PTR_STRING(value,size) { TYPE_STRING, value, size }
#define DATA_PTR_STRING_AUTO(value) { TYPE_STRING, value, strlen(value)+1 }
#define DT_STRING                   char *
#define GET_TYPE_STRING(value)     ((char *)value->data_ptr)

#endif
