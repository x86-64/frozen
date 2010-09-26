#include <libfrozen.h>

// TODO rewrite _cmp using memcmp, or similar
// TODO rewrite using switch() with actions and simple stub functions
// TODO overflow checks look strange, is it right?
// TODO register_data not multilined

/*
m4_define(`BYTES', m4_eval(BITS() `/ 8'))
m4_ifelse(BYTES(), `1', `m4_define(`TYPE', `char')')
m4_ifelse(BYTES(), `2', `m4_define(`TYPE', `short')')
m4_ifelse(BYTES(), `4', `m4_define(`TYPE', `int')')
m4_ifelse(BYTES(), `8', `m4_define(`TYPE', `long long')')
*/

`int data_int'BITS()_cmp(void *data1, void *data2){ // {{{
	int           cret;
	unsigned TYPE data1_val, data2_val;
	
	cret      = 0;
	data1_val = *(unsigned TYPE *)data1;
	data2_val = *(unsigned TYPE *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

`int data_int'BITS()_inc(void *data){ // {{{
	unsigned TYPE data_val;
	
	data_val = *(unsigned TYPE *)data;
	if(data_val == 0xFF)
		return -1;
	
	*(unsigned char *)data = data_val + 1;
	return 0;
} // }}}

`int data_int'BITS()_div(void *data, unsigned int divider){ // {{{
	if(divider == 0)
		return -1;
	
	unsigned TYPE data_val = *(unsigned TYPE *)data;
	*(unsigned TYPE *)data = data_val / divider;
	
	return 0;
} // }}}

`int data_int'BITS()_mul(void *data, unsigned int mul){ // {{{
	unsigned TYPE data_val  = *(unsigned TYPE *)data;
	unsigned TYPE data_muls = data_val * mul;
	
	if(data_val > data_muls) // check for overflow
		return -1; 
	
	*(unsigned TYPE *)data = data_muls;
	return 0;
} // }}}

REGISTER_DATA(`TYPE_INT'BITS(),SIZE_FIXED, .func_cmp = &`data_int'BITS()_cmp, .func_inc = &`data_int'BITS()_inc, .func_div = &`data_int'BITS()_div, .func_mul = &`data_int'BITS()_mul, .fixed_size = BYTES)

/* vim: set filetype=c: */
