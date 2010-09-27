#line 1 "data/uint.c.m4"
#include <libfrozen.h>

// TODO rewrite _cmp using memcmp, or similar
// TODO rewrite using switch() with actions and simple stub functions
// TODO overflow checks look strange, is it right?
// TODO register_data not multilined

/*





*/

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

int data_int8_add(void *data, unsigned int number){ // {{{
	unsigned char data_val = *(unsigned char *)data;
	*(unsigned char *)data = data_val + number;
	return 0;
} // }}}

int data_int8_sub(void *data, unsigned int number){ // {{{
	unsigned char data_val = *(unsigned char *)data;
	*(unsigned char *)data = data_val - number;
	return 0;
} // }}}

int data_int8_div(void *data, unsigned int divider){ // {{{
	if(divider == 0)
		return -1;
	
	unsigned char data_val = *(unsigned char *)data;
	*(unsigned char *)data = data_val / divider;
	
	return 0;
} // }}}

int data_int8_mul(void *data, unsigned int mul){ // {{{
	unsigned char data_val  = *(unsigned char *)data;
	unsigned char data_muls = data_val * mul;
	
	if(data_val > data_muls) // check for overflow
		return -1; 
	
	*(unsigned char *)data = data_muls;
	return 0;
} // }}}

REGISTER_DATA(TYPE_INT8,SIZE_FIXED, .func_cmp = &data_int8_cmp, .func_add = &data_int8_add, .func_sub = &data_int8_sub, .func_div = &data_int8_div, .func_mul = &data_int8_mul, .fixed_size = 1)

/* vim: set filetype=c: */
