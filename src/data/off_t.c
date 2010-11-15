#line 1 "data/uint.c.m4"
#include <libfrozen.h>

/*





 
 

*/
#ifndef __MAX
	#define __HALF_MAX_SIGNED(type) ((type)1 << (sizeof(type)*8-2))
	#define __MAX_SIGNED(type) (__HALF_MAX_SIGNED(type) - 1 + __HALF_MAX_SIGNED(type))
	#define __MIN_SIGNED(type) (-1 - __MAX_SIGNED(type))
	#define __MIN(type) ((type)-1 < 1?__MIN_SIGNED(type):(type)0)
	#define __MAX(type) ((type)~__MIN(type))
#endif

int data_off_t_cmp(data_t *data1, data_ctx_t *ctx1, data_t *data2, data_ctx_t *ctx2){ // {{{
	int           cret;
	off_t          data1_val, data2_val;
	
	if(data1->data_size < 8 || data2->data_size < 8)
		return -EINVAL;
	
	data1_val = *(off_t *)(data1->data_ptr);
	data2_val = *(off_t *)(data2->data_ptr); 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}

int data_off_t_arith(char operator, data_t *operand1, data_ctx_t *ctx1, data_t *operand2, data_ctx_t *ctx2){ // {{{
	int           ret = 0;
	off_t          operand1_val, operand2_val, result;
	
	if(operand1->data_size < 8 || operand2->data_size < 8)
		return -EINVAL;
	
	operand1_val = *(off_t *)(operand1->data_ptr);
	operand2_val = *(off_t *)(operand2->data_ptr); 
	switch(operator){
		case '+':
			if(__MAX(off_t) - operand1_val < operand2_val)
				ret = -EOVERFLOW;
			
			result = operand1_val + operand2_val;
			break;
		case '-':
			if(__MIN(off_t) + operand2_val > operand1_val)
				ret = -EOVERFLOW;
			
			result = operand1_val - operand2_val;
			break;
		case '*':
			if(operand2_val == 0){
				result = 0;
			}else{
				if(operand1_val > __MAX(off_t) / operand2_val)
					ret = -EOVERFLOW;
				
				result = operand1_val * operand2_val;
			}
			break;
		case '/':
			if(operand2_val == 0)
				return -EINVAL;
			
			result = operand1_val / operand2_val;
			break;
		default:
			return -1;
	}
	*(off_t *)(operand1->data_ptr) = result;
	return ret;
} // }}}

/*
REGISTER_TYPE(TYPE_OFFT)
REGISTER_MACRO(DATA_OFFT(value),     `TYPE_OFFT, (off_t []){ value }, sizeof(off_t)')
REGISTER_MACRO(DATA_PTR_OFFT(value), `TYPE_OFFT, value, sizeof(off_t)')
REGISTER_PROTO(
	`{
		.type                   = TYPE_OFFT,
		.type_str               = "off_t",
		.size_type              = SIZE_FIXED,
		.func_cmp               = &data_off_t_cmp,
		.func_arithmetic        = &data_off_t_arith,
		.fixed_size             = 8
	}'
)
*/
/* vim: set filetype=c: */
