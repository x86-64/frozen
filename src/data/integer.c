#include <libfrozen.h>

// TODO rewite using memcmp, or similar

/* Integer - 8bit {{{ */
int data_int8_cmp(void *data1, void *data2){ // {{{
	int           cret;
	unsigned char data1_val, data2_val;
	
	cret      = 0;
	data1_val = *(unsigned char *)data1;
	data2_val = *(unsigned char *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}
int data_int8_inc(void *data){ // {{{
	unsigned char data_val;
	
	data_val = *(unsigned char *)data;
	if(data_val == 0xFF)
		return -1;
	
	*(unsigned char *)data = data_val + 1;
	return 0;
} // }}}
int data_int8_div(void *data, unsigned int divider){ // {{{
	return -1;
} // }}}

REGISTER_DATA(TYPE_INT8,SIZE_FIXED, .func_cmp = &data_int8_cmp, .func_inc = &data_int8_inc, .fixed_size = 1)
/* }}} */

/* Integer - 16bit {{{ */
int data_int16_cmp(void *data1, void *data2){ // {{{
	int            cret;
	unsigned short data1_val, data2_val;
	
	cret      = 0;
	data1_val = *(unsigned short *)data1;
	data2_val = *(unsigned short *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}
int data_int16_inc(void *data){ // {{{
	unsigned short data_val;
	
	data_val = *(unsigned short *)data;
	if(data_val == 0xFFFF)
		return -1;
	
	*(unsigned short *)data = data_val + 1;
	return 0;
} // }}}
int data_int16_div(void *data, unsigned int divider){ // {{{
	return -1;	
} // }}}

REGISTER_DATA(TYPE_INT16,SIZE_FIXED, .func_cmp = &data_int16_cmp, .func_inc = &data_int16_inc, .fixed_size = 2)
/* }}} */

/* Integer - 32bit {{{ */
int data_int32_cmp(void *data1, void *data2){ // {{{
	int           cret;
	unsigned int  data1_val, data2_val;
	
	cret      = 0;
	data1_val = *(unsigned int *)data1;
	data2_val = *(unsigned int *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}
int data_int32_inc(void *data){ // {{{
	unsigned int data_val;
	
	data_val = *(unsigned int *)data;
	if(data_val == 0xFFFFFFFF)
		return -1;
	
	*(unsigned int *)data = data_val + 1;
	return 0;
} // }}}
int data_int32_div(void *data, unsigned int divider){ // {{{
	return -1;
} // }}}

REGISTER_DATA(TYPE_INT32,SIZE_FIXED, .func_cmp = &data_int32_cmp, .func_inc = &data_int32_inc, .fixed_size = 4)
/* }}} */

/* Integer - 64bit {{{ */
int data_int64_cmp(void *data1, void *data2){ // {{{
	int                 cret;
	unsigned long long  data1_val, data2_val;
	
	cret      = 0;
	data1_val = *(unsigned long long *)data1;
	data2_val = *(unsigned long long *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}
int data_int64_inc(void *data){ // {{{
	unsigned long long data_val;
	
	data_val = *(unsigned long long *)data;
	if(data_val == (unsigned long long)-1)
		return -1;
	
	*(unsigned long long *)data = data_val + 1;
	return 0;
} // }}}
int data_int64_div(void *data, unsigned int divider){ // {{{
	return -1;
} // }}}

REGISTER_DATA(TYPE_INT64,SIZE_FIXED, .func_cmp = &data_int64_cmp, .func_inc = &data_int64_inc, .fixed_size = 8)
/* }}} */

