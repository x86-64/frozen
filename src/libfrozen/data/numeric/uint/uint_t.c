#line 1 "uint.c.m4"
#include <libfrozen.h>

#line 1 "uint_init.m4"






















#line 3 "uint.c.m4"


#include <uint_t.h>
#include <enum/format/format_t.h>

uintmax_t * data_uint_t_alloc(uintmax_t value){ // {{{
	uintmax_t        *ptr;
	
	if( (ptr = malloc(sizeof(uintmax_t))) == NULL)
		return NULL;
	
	*ptr = value;
	return ptr;
} // }}}

static ssize_t data_uint_t_len(data_t *data, fastcall_length *fargs){ // {{{
	fargs->length = sizeof(uintmax_t);
	return 0;
} // }}}
static ssize_t data_uint_t_resize(data_t *data, fastcall_resize *fargs){ // {{{
	if(data->ptr)
		return 0;

	if( (data->ptr = malloc(sizeof(uintmax_t))) == NULL)
		return -ENOMEM;
	return 0;
} // }}}
static ssize_t data_uint_t_compare(data_t *data1, fastcall_compare *fargs){ // {{{
	int           cret;
	uintmax_t          data1_val, data2_val;
	
	if(fargs->data2 == NULL)
		return -EINVAL;
	
	data1_val = *(uintmax_t *)(data1->ptr);
	data2_val = *(uintmax_t *)(fargs->data2->ptr); 
	     if(data1_val == data2_val){ cret =  0; }
	else if(data1_val <  data2_val){ cret =  1; }
	else                           { cret =  2; }
	
	fargs->result = cret;
	return 0;
} // }}}
static ssize_t data_uint_t_arith(data_t *data1, fastcall_arith *fargs){ // {{{
	int           ret = 0;
	uintmax_t          operand1_val, operand2_val, result;
	
	if(fargs->data2 == NULL)
		return -EINVAL;
	
	operand1_val = *(uintmax_t *)(data1->ptr);
	operand2_val = *(uintmax_t *)(fargs->data2->ptr); 
	switch(fargs->header.action){
		case ACTION_ADD:
			if(__MAX(uintmax_t) - operand1_val < operand2_val)
				ret = -EOVERFLOW;
			
			result = operand1_val + operand2_val;
			break;
		case ACTION_SUB:
			if(__MIN(uintmax_t) + operand2_val > operand1_val)
				ret = -EOVERFLOW;
			
			result = operand1_val - operand2_val;
			break;
		case ACTION_MULTIPLY:
			if(operand2_val == 0){
				result = 0;
			}else{
				if(operand1_val > __MAX(uintmax_t) / operand2_val)
					ret = -EOVERFLOW;
				
				result = operand1_val * operand2_val;
			}
			break;
		case ACTION_DIVIDE:
			if(operand2_val == 0)
				return -EINVAL;
			
			result = operand1_val / operand2_val;
			break;
		default:
			return -1;
	}
	*(uintmax_t *)(data1->ptr) = result;
	return ret;
} // }}}
static ssize_t data_uint_t_arith_no_arg(data_t *data1, fastcall_arith_no_arg *fargs){ // {{{
	int           ret = 0;
	uintmax_t          operand1_val, result;
	
	operand1_val = *(uintmax_t *)(data1->ptr);
	switch(fargs->header.action){
		case ACTION_INCREMENT:
			if(__MAX(uintmax_t) - operand1_val < 1)
				ret = -EOVERFLOW;
			
			result = operand1_val + 1;
			break;
		case ACTION_DECREMENT:
			if(__MIN(uintmax_t) + 1 > operand1_val)
				ret = -EOVERFLOW;
			
			result = operand1_val - 1;
			break;
		default:
			return -1;
	}
	*(uintmax_t *)(data1->ptr) = result;
	return ret;
} // }}}
static ssize_t data_uint_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              transfered        = 0;
	char                   buffer[DEF_BUFFER_SIZE];
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	switch( fargs->format ){
		case FORMAT(native):;
		case FORMAT(packed):;
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, src->ptr, sizeof(uintmax_t) };
			ret        = data_query(fargs->dest, &r_write);
			transfered = r_write.buffer_size;
			break;
		
		case FORMAT(human):
		case FORMAT(config):
			if( (transfered = snprintf(
				buffer, sizeof(buffer),
				"%" PRIuMAX,
				*(uintmax_t *)src->ptr
			)) >= sizeof(buffer))
				return -ENOMEM;
			
			fastcall_write r_write2 = { { 5, ACTION_WRITE }, 0, buffer, transfered };
			ret        = data_query(fargs->dest, &r_write2);
			transfered = r_write2.buffer_size;
			break;
			
		default:
			return -ENOSYS;
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_uint_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	char                  *endptr;
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	uintmax_t              transfered                = 0;
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(uintmax_t))) == NULL)
			return -ENOMEM;
	}

	switch( fargs->format ){
		case FORMAT(config):;
		case FORMAT(human):; // TODO fix it for slider_t 
			fastcall_read r_read_str = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if( (ret = data_query(fargs->src, &r_read_str)) < 0){
				// TODO memleak
				return ret;
			}
			
			*(uintmax_t *)(dst->ptr) = (uintmax_t )strtoul(buffer, &endptr, 10);
			transfered = (endptr - buffer);
			break;

		case FORMAT(native):;
		case FORMAT(packed):;
			if(fargs->src->type == dst->type){
				*(uintmax_t *)(dst->ptr) = *(uintmax_t *)(fargs->src->ptr);
				ret = 0;
			}else{
				fastcall_read r_read = { { 5, ACTION_READ }, 0, &buffer, sizeof(uintmax_t) };
				if( (ret = data_query(fargs->src, &r_read)) < 0){
					// TODO memleak
					return ret;
				}
				
				*(uintmax_t *)(dst->ptr) = *((uintmax_t *)buffer);
			}
			transfered = sizeof(uintmax_t);
			break;
			
		default:
			return -ENOSYS;
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return 0;
} // }}}
static ssize_t data_uint_t_is_null(data_t *data, fastcall_is_null *fargs){ // {{{
	fargs->is_null = (data->ptr == NULL || *(uintmax_t *)data->ptr == 0) ? 1 : 0;
	return 0;
} // }}}

data_proto_t uint_t_proto = {
	.type                   = TYPE_UINTT,
	.type_str               = "uint_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handlers               = {
		[ACTION_RESIZE]         = (f_data_func)&data_uint_t_resize,
		[ACTION_LENGTH]         = (f_data_func)&data_uint_t_len,
		[ACTION_COMPARE]        = (f_data_func)&data_uint_t_compare,
		[ACTION_ADD]            = (f_data_func)&data_uint_t_arith,
		[ACTION_SUB]            = (f_data_func)&data_uint_t_arith,
		[ACTION_MULTIPLY]       = (f_data_func)&data_uint_t_arith,
		[ACTION_DIVIDE]         = (f_data_func)&data_uint_t_arith,
		[ACTION_INCREMENT]      = (f_data_func)&data_uint_t_arith_no_arg,
		[ACTION_DECREMENT]      = (f_data_func)&data_uint_t_arith_no_arg,
		[ACTION_CONVERT_TO]     = (f_data_func)&data_uint_t_convert_to,
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_uint_t_convert_from,
		[ACTION_IS_NULL]        = (f_data_func)&data_uint_t_is_null,
	}
};
/* vim: set filetype=m4: */
