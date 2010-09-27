#include <libfrozen.h>

// simply use existing functions

OID_REGISTER(`{ TYPE_INT8,  &data_int8_add,  &data_int8_div , &data_int8_mul  }')
OID_REGISTER(`{ TYPE_INT16, &data_int16_add, &data_int16_div, &data_int16_mul }')
OID_REGISTER(`{ TYPE_INT32, &data_int32_add, &data_int32_div, &data_int32_mul }') 
OID_REGISTER(`{ TYPE_INT64, &data_int64_add, &data_int64_div, &data_int64_mul }')

