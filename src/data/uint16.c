#line 1 "data/uint.c.m4"
#include <libfrozen.h>

// TODO rewrite _cmp using memcmp, or similar
// TODO rewrite using switch() with actions and simple stub functions
// TODO overflow checks look strange, is it right?

/*






*/

int data_int16_buff_cmp(data_ctx_t *ctx, buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off){ // {{{
	int           cret;
	unsigned short data1_val = 0;
	unsigned short data2_val = 0;
	
	buffer_read(buffer1, buffer1_off, &data1_val, sizeof(data1_val));
	buffer_read(buffer2, buffer2_off, &data2_val, sizeof(data2_val));
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

int data_int16_cmp(void *data1, void *data2){ // {{{
	int           cret;
	unsigned short data1_val, data2_val;
	
	data1_val = *(unsigned short *)data1;
	data2_val = *(unsigned short *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

int data_int16_bare_arith(void *data, char operator, unsigned int operand){ // {{{
	unsigned short data_val = *(unsigned short *)data;
	unsigned short data_tmp;
	
	switch(operator){
		case '+': *(unsigned short *)data = data_val + operand; return 0;
		case '-': *(unsigned short *)data = data_val - operand; return 0;
		case '*':
			data_tmp = data_val * operand;
			if(data_val > data_tmp)
				return -1;
			*(unsigned short *)data = data_tmp;
			return 0;
		case '/':
			if(operand == 0)
				return -1;
			*(unsigned short *)data = data_val / operand;
			return 0;
	}
	return -1;
} // }}}

int data_int16_buff_arith(data_ctx_t *ctx, buffer_t *buffer, off_t buffer_off, char operator, unsigned int operand){ // {{{
	int           ret;
	unsigned short data_val;
	
	if(buffer_read  (buffer, buffer_off, &data_val, sizeof(data_val)) != sizeof(data_val))
		return -1;
	ret = data_int16_bare_arith(&data_val, operator, operand);
	if(buffer_write (buffer, buffer_off, &data_val, sizeof(data_val)) != sizeof(data_val))
		return -1;
	
	return ret;
} // }}}

/*
REGISTER_TYPE(TYPE_INT16)
REGISTER_PROTO(
	`{
		.type                   = TYPE_INT16,
		.type_str               = "int16",
		.size_type              = SIZE_FIXED,
		.func_bare_cmp          = &data_int16_cmp,
		.func_bare_arithmetic   = &data_int16_bare_arith,
		.func_buffer_cmp        = &data_int16_buff_cmp,
		.func_buffer_arithmetic = &data_int16_buff_arith,
		.fixed_size             = 2
	}'
)
*/
/* vim: set filetype=c: */
