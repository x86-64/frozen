#include <libfrozen.h>

m4_include(uint_init.m4)

[#include <]NAME()[.h>]

static int data_[]NAME()_cmp(data_t *data1, data_ctx_t *ctx1, data_t *data2, data_ctx_t *ctx2){ // {{{
	int           cret;
	TYPE          data1_val, data2_val;
	
	(void)ctx1; (void)ctx2; // TODO use ctx
	
	if(data1->data_size < sizeof(data1_val) || data2->data_size < sizeof(data2_val))
		return -EINVAL;
	
	data1_val = *(TYPE *)(data1->data_ptr);
	data2_val = *(TYPE *)(data2->data_ptr); 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret = -1; }
	else                           { cret =  1; }
	
	return cret;
} // }}}
static int data_[]NAME()_arith(char operator, data_t *operand1, data_ctx_t *ctx1, data_t *operand2, data_ctx_t *ctx2){ // {{{
	int           ret = 0;
	TYPE          operand1_val, operand2_val, result;
	
	(void)ctx1; (void)ctx2; // TODO use ctx
	
	if(operand1->data_size < sizeof(operand1_val) || operand2->data_size < sizeof(operand2_val))
		return -EINVAL;
	
	operand1_val = *(TYPE *)(operand1->data_ptr);
	operand2_val = *(TYPE *)(operand2->data_ptr); 
	switch(operator){
		case '+':
			if(__MAX(TYPE) - operand1_val < operand2_val)
				ret = -EOVERFLOW;
			
			result = operand1_val + operand2_val;
			break;
		case '-':
			if(__MIN(TYPE) + operand2_val > operand1_val)
				ret = -EOVERFLOW;
			
			result = operand1_val - operand2_val;
			break;
		case '*':
			if(operand2_val == 0){
				result = 0;
			}else{
				if(operand1_val > __MAX(TYPE) / operand2_val)
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
	*(TYPE *)(operand1->data_ptr) = result;
	return ret;
} // }}}
static ssize_t data_[]NAME()_convert(data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){ // {{{
	char                  buffer[[DEF_BUFFER_SIZE]];
	TYPE                  value = 0;
	
	switch(src->type){
	#ifdef TYPE_STRING
		case TYPE_STRING:
			if(data_read(src, src_ctx, 0, &buffer, DEF_BUFFER_SIZE) < 0)
				return -EINVAL;
			
			value = (TYPE )strtoul(buffer, NULL, 10);
			
			data_write(dst, dst_ctx, 0, &value, sizeof(value));
			return 0;
	#endif
	#ifdef TYPE_UINT8T
		case TYPE_UINT8T:
	#endif
	#ifdef TYPE_UINT16T
		case TYPE_UINT16T:
	#endif
	#ifdef TYPE_UINT32T
		case TYPE_UINT32T:
	#endif
	#ifdef TYPE_UINT64T
		case TYPE_UINT64T:
	#endif
	#ifdef TYPE_OFFT
		case TYPE_OFFT:
	#endif
	#ifdef TYPE_SIZET
		case TYPE_SIZET:
	#endif
			data_write(dst, dst_ctx, 0, &value, sizeof(value));
			return (data_transfer(dst, dst_ctx, src, src_ctx) > 0) ? 0 : -EFAULT;
		default:
			break;
	};
	return -ENOSYS;
} // }}}

data_proto_t NAME()_proto = {
		.type                   = [TYPE_]DEF()[,
		.type_str               = "]NAME()[",
		.size_type              = SIZE_FIXED,
		.func_cmp               = &data_]NAME()[_cmp,
		.func_arithmetic        = &data_]NAME()[_arith,
		.func_convert           = &data_]NAME()[_convert,
		.fixed_size             = sizeof(]TYPE()[)]
};
/* vim: set filetype=m4: */
