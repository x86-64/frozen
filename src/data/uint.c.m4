#include <libfrozen.h>

// TODO rewrite _cmp using memcmp, or similar
// TODO rewrite using switch() with actions and simple stub functions
// TODO overflow checks look strange, is it right?

/*
m4_define(`BYTES', m4_eval(BITS() `/ 8'))
m4_ifelse(BYTES(), `1', `m4_define(`TYPE', `char')')
m4_ifelse(BYTES(), `2', `m4_define(`TYPE', `short')')
m4_ifelse(BYTES(), `4', `m4_define(`TYPE', `int')')
m4_ifelse(BYTES(), `8', `m4_define(`TYPE', `long long')')
m4_changequote([,])
*/

[int data_int]BITS()_buff_cmp(data_ctx_t *ctx, buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off){ // {{{
	int           cret;
	unsigned TYPE data1_val = 0;
	unsigned TYPE data2_val = 0;
	
	buffer_read(buffer1, buffer1_off, &data1_val, sizeof(data1_val));
	buffer_read(buffer2, buffer2_off, &data2_val, sizeof(data2_val));
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

[int data_int]BITS()_cmp(void *data1, void *data2){ // {{{
	int           cret;
	unsigned TYPE data1_val, data2_val;
	
	data1_val = *(unsigned TYPE *)data1;
	data2_val = *(unsigned TYPE *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

[int data_int]BITS()_bare_arith(void *data, char operator, unsigned int operand){ // {{{
	unsigned TYPE data_val = *(unsigned TYPE *)data;
	unsigned TYPE data_tmp;
	
	switch(operator){
		case '+': *(unsigned TYPE *)data = data_val + operand; return 0;
		case '-': *(unsigned TYPE *)data = data_val - operand; return 0;
		case '*':
			data_tmp = data_val * operand;
			if(data_val > data_tmp)
				return -1;
			*(unsigned TYPE *)data = data_tmp;
			return 0;
		case '/':
			if(operand == 0)
				return -1;
			*(unsigned TYPE *)data = data_val / operand;
			return 0;
	}
	return -1;
} // }}}

[int data_int]BITS()_buff_arith(data_ctx_t *ctx, buffer_t *buffer, off_t buffer_off, char operator, unsigned int operand){ // {{{
	int           ret;
	unsigned TYPE data_val;
	
	if(buffer_read  (buffer, buffer_off, &data_val, sizeof(data_val)) != sizeof(data_val))
		return -1;
	ret = data_int[]BITS()_bare_arith(&data_val, operator, operand);
	if(buffer_write (buffer, buffer_off, &data_val, sizeof(data_val)) != sizeof(data_val))
		return -1;
	
	return ret;
} // }}}

/*
REGISTER_TYPE([TYPE_INT]BITS())
REGISTER_PROTO([
	`{
		.type                   = TYPE_INT]BITS()[,
		.type_str               = "int]BITS()[",
		.size_type              = SIZE_FIXED,
		.func_bare_cmp          = &data_int]BITS()[_cmp,
		.func_bare_arithmetic   = &data_int]BITS()[_bare_arith,
		.func_buffer_cmp        = &data_int]BITS()[_buff_cmp,
		.func_buffer_arithmetic = &data_int]BITS()[_buff_arith,
		.fixed_size             = ]BYTES()[
	}'
])
*/
/* vim: set filetype=c: */
