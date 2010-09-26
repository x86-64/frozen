#line 1 "data/uint.c.m4"
#include <libfrozen.h>

// TODO rewrite _cmp using memcmp, or similar
// TODO rewrite using switch() with actions and simple stub functions
// TODO overflow checks look strange, is it right?
// TODO register_data not multilined

/*





*/

int data_int64_cmp(void *data1, void *data2){ // {{{
	int           cret;
	unsigned long long data1_val, data2_val;
	
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
	if(data_val == 0xFF)
		return -1;
	
	*(unsigned char *)data = data_val + 1;
	return 0;
} // }}}

int data_int64_div(void *data, unsigned int divider){ // {{{
	if(divider == 0)
		return -1;
	
	unsigned long long data_val = *(unsigned long long *)data;
	*(unsigned long long *)data = data_val / divider;
	
	return 0;
} // }}}

int data_int64_mul(void *data, unsigned int mul){ // {{{
	unsigned long long data_val  = *(unsigned long long *)data;
	unsigned long long data_muls = data_val * mul;
	
	if(data_val > data_muls) // check for overflow
		return -1; 
	
	*(unsigned long long *)data = data_muls;
	return 0;
} // }}}

REGISTER_DATA(TYPE_INT64,SIZE_FIXED, .func_cmp = &data_int64_cmp, .func_inc = &data_int64_inc, .func_div = &data_int64_div, .func_mul = &data_int64_mul, .fixed_size = 8)

/* vim: set filetype=c: */
