#line 1 "data/uint.c.m4"
#include <libfrozen.h>

// TODO rewrite _cmp using memcmp, or similar
// TODO rewrite using switch() with actions and simple stub functions
// TODO overflow checks look strange, is it right?

/*






*/

int data_int32_buff_cmp(data_ctx_t *ctx, buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off){ // {{{
	int           cret;
	unsigned int data1_val = 0;
	unsigned int data2_val = 0;
	
	buffer_read(buffer1, buffer1_off, &data1_val, sizeof(data1_val));
	buffer_read(buffer2, buffer2_off, &data2_val, sizeof(data2_val));
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

int data_int32_cmp(void *data1, void *data2){ // {{{
	int           cret;
	unsigned int data1_val, data2_val;
	
	data1_val = *(unsigned int *)data1;
	data2_val = *(unsigned int *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

int data_int32_add(void *data, unsigned int number){ // {{{
	unsigned int data_val = *(unsigned int *)data;
	*(unsigned int *)data = data_val + number;
	return 0;
} // }}}

int data_int32_sub(void *data, unsigned int number){ // {{{
	unsigned int data_val = *(unsigned int *)data;
	*(unsigned int *)data = data_val - number;
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

/*
REGISTER_TYPE(TYPE_INT32)
REGISTER_PROTO(
	`{
		.type            = TYPE_INT32,
		.type_str        = "int32",
		.size_type       = SIZE_FIXED,
		.func_bare_cmp   = &data_int32_cmp,
		.func_buffer_cmp = &data_int32_buff_cmp,
		.func_add        = &data_int32_add,
		.func_sub        = &data_int32_sub,
		.func_div        = &data_int32_div,
		.func_mul        = &data_int32_mul,
		.fixed_size      = 4
	}'
)
*/
/* vim: set filetype=c: */
