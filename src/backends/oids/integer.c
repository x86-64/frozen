#include <libfrozen.h>

// simply use existing functions

OID_REGISTER(`{ TYPE_INT8,  &data_int8_inc,  &data_int8_div  }')
OID_REGISTER(`{ TYPE_INT16, &data_int16_inc, &data_int16_div }')
OID_REGISTER(`{ TYPE_INT32, &data_int32_inc, &data_int32_div }') 
OID_REGISTER(`{ TYPE_INT64, &data_int64_inc, &data_int64_div }')

