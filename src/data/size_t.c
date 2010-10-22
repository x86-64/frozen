#include <libfrozen.h>

int data_sizet_buff_cmp(data_ctx_t *ctx, buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off){ // {{{
	int           cret;
	size_t data1_val = 0;
	size_t data2_val = 0;
	
	buffer_read(buffer1, buffer1_off, &data1_val, sizeof(data1_val));
	buffer_read(buffer2, buffer2_off, &data2_val, sizeof(data2_val));
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

int data_sizet_cmp(void *data1, void *data2){ // {{{
	int           cret;
	size_t         data1_val, data2_val;
	
	data1_val = *(size_t *)data1;
	data2_val = *(size_t *)data2; 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

int data_sizet_bare_arith(void *data, char operator, unsigned int operand){ // {{{
	size_t data_val = *(size_t *)data;
	size_t data_tmp;
	
	switch(operator){
		case '+': *(size_t *)data = data_val + operand; return 0;
		case '-': *(size_t *)data = data_val - operand; return 0;
		case '*':
			data_tmp = data_val * operand;
			if(data_val > data_tmp)
				return -1;
			*(size_t *)data = data_tmp;
			return 0;
		case '/':
			if(operand == 0)
				return -1;
			*(size_t *)data = data_val / operand;
			return 0;
	}
	return -1;
} // }}}

int data_sizet_buff_arith(data_ctx_t *ctx, buffer_t *buffer, off_t buffer_off, char operator, unsigned int operand){ // {{{
	int           ret;
	size_t data_val;
	
	if(buffer_read  (buffer, buffer_off, &data_val, sizeof(data_val)) != sizeof(data_val))
		return -1;
	ret = data_sizet_bare_arith(&data_val, operator, operand);
	if(buffer_write (buffer, buffer_off, &data_val, sizeof(data_val)) != sizeof(data_val))
		return -1;
	
	return ret;
} // }}}

/*
REGISTER_TYPE(TYPE_SIZET)
REGISTER_MACRO(`DATA_SIZET(value)',     `TYPE_SIZET, (size_t []){ value }, sizeof(size_t)')
REGISTER_MACRO(`DATA_PTR_SIZET(value)', `TYPE_SIZET, value, sizeof(size_t)')
REGISTER_PROTO(
	`{
		.type                   = TYPE_SIZET,
		.type_str               = "size_t",
		.size_type              = SIZE_FIXED,
		.func_bare_cmp          = &data_sizet_cmp,
		.func_bare_arithmetic   = &data_sizet_bare_arith,
		.func_buffer_cmp        = &data_sizet_buff_cmp,
		.func_buffer_arithmetic = &data_sizet_buff_arith,
		.fixed_size             = 8
	}'
)
*/
/* vim: set filetype=c: */
