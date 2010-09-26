#line 1 "data/uint.c.m4"
#include <libfrozen.h>

// TODO rewrite _cmp using memcmp, or similar
// TODO rewrite using switch() with actions and simple stub functions
// TODO overflow checks look strange, is it right?
// TODO register_data not multilined

/*





*/

int data_int32_cmp(void *data1, void *data2){ // {{{
	int           cret;
	unsigned int data1_val, data2_val;
	
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
	if(data_val == 0xFF)
		return -1;
	
	*(unsigned char *)data = data_val + 1;
	return 0;
} // }}}

int data_int32_div(void *data, unsigned int divider){ // {{{
	if(divider == 0)
		return -1;
	
	unsigned int data_val = *(unsigned int *)data;
	*(unsigned int *)data = data_val / divider;
	
	return 0;
} // }}}

int data_int32_mul(void *data, unsigned int mul){ // {{{
	unsigned int data_val  = *(unsigned int *)data;
	unsigned int data_muls = data_val * mul;
	
	if(data_val > data_muls) // check for overflow
		return -1; 
	
	*(unsigned int *)data = data_muls;
	return 0;
} // }}}

REGISTER_DATA(TYPE_INT32,SIZE_FIXED, .func_cmp = &data_int32_cmp, .func_inc = &data_int32_inc, .func_div = &data_int32_div, .func_mul = &data_int32_mul, .fixed_size = 4)

/* vim: set filetype=c: */
