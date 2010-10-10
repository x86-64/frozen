#include <libfrozen.h>

/* m4 {{{
m4_define(`REGISTER_PROTO', `
        m4_define(`VAR_ARRAY', m4_defn(`VAR_ARRAY')`$1,
')')
}}} */

m4_include(data_protos.m4)

/* data protos {{{ */
data_proto_t data_protos[]    = { VAR_ARRAY() };
size_t       data_protos_size = sizeof(data_protos) / sizeof(data_proto_t);
// }}}

int                  data_type_is_valid     (data_type type){ // {{{
	if(type != -1 && (unsigned)type < data_protos_size){
		return 1;
	}
	return 0;
} // }}}
data_type            data_type_from_string  (char *string){ // {{{
	int i;
	
	for(i=0; i<data_protos_size; i++){
		if(strcasecmp(data_protos[i].type_str, string) == 0)
			return data_protos[i].type;
	}
	
	return -1;
} // }}}
data_proto_t *       data_proto_from_type   (data_type type){ // {{{
	if((unsigned)type >= data_protos_size)
		return 0;
	
	return &data_protos[type];
} // }}}

int                  data_ctx_init          (data_ctx_t *ctx, data_type type, void *context){ // {{{
	f_data_ctx_n   func_ctx_parse;
	void          *user_data;
	
	if((unsigned)type >= data_protos_size)
		return -EINVAL;
	
	user_data = NULL;
	if( (func_ctx_parse = data_protos[type].func_ctx_parse) != NULL){
		user_data = func_ctx_parse(context);
	}
	
	ctx->data_proto     = &(data_protos[type]);
	ctx->user_data      = user_data;
	return 0;
} // }}}
int                  data_ctx_destory       (data_ctx_t *ctx){ // {{{
	if( ctx->data_proto->func_ctx_free != NULL)
		ctx->data_proto->func_ctx_free(ctx->user_data);
	
	return 0;
} // }}}

size_t               data_buffer_len        (data_ctx_t *ctx, buffer_t *buffer, off_t buffer_off){ // {{{
	f_data_len_b   func_len;
	
	if(ctx == NULL || buffer == NULL)
		return 0;
	
	switch(ctx->data_proto->size_type){
		case SIZE_FIXED:
			return ctx->data_proto->fixed_size; // TODO respect buffer size, struct.c need rewrite for this
			
		case SIZE_VARIABLE:
			if( (func_len = ctx->data_proto->func_buffer_len) == NULL)
				return 0;
			
			return func_len(ctx, buffer, buffer_off);
	}
	return 0;
} // }}}
int                  data_buffer_cmp        (data_ctx_t *ctx, buffer_t *buffer1, off_t buffer1_off, buffer_t *buffer2, off_t buffer2_off){ // {{{
	f_data_cmp_b   func_cmp;
	
	if(ctx == NULL || buffer1 == NULL || buffer2 == NULL)
		return -EINVAL;
	
	if( (func_cmp = ctx->data_proto->func_buffer_cmp) == NULL)
		return -EINVAL;
	
	return func_cmp(ctx, buffer1, buffer1_off, buffer2, buffer2_off);
} // }}}
int                  data_buffer_arithmetic (data_ctx_t *ctx, buffer_t *buffer, off_t buffer_off, char operator, unsigned int operand){ // {{{
	f_data_arith_b  func_arith;
	
	if(ctx == NULL || buffer == NULL)
		return -EINVAL;
	
	if( (func_arith = ctx->data_proto->func_buffer_arithmetic) == NULL)
		return -EINVAL;
	
	return func_arith(ctx, buffer, buffer_off, operator, operand);
} // }}}

size_t               data_bare_len          (data_type type, data_t *data, size_t buffer_size){ // {{{
	f_data_len_r   func_len;
	
	if((unsigned)type >= data_protos_size)
		return 0;
	
	switch(data_protos[type].size_type){
		case SIZE_FIXED:
			return data_protos[type].fixed_size; // TODO respect buffer size, struct.c need rewrite for this
			
		case SIZE_VARIABLE:
			if( (func_len = data_protos[type].func_bare_len) == NULL || data == NULL)
				return 0;
					
			return func_len(data, buffer_size);
	}
	return 0;
} // }}}
int                  data_bare_cmp          (data_type type, data_t *data1, data_t *data2){ // {{{
	f_data_cmp_r   func_cmp;
	
	if((unsigned)type >= data_protos_size)
		return 0;
	
	if( (func_cmp = data_protos[type].func_bare_cmp) == NULL)
		return 0;
	
	return func_cmp(data1, data2);
} // }}}
int                  data_bare_arithmetic   (data_type type, data_t *data, char operator, unsigned int operand){ // {{{
	f_data_arith_r  func_arith;
	
	if((unsigned)type >= data_protos_size)
		return 0;
	
	if( (func_arith = data_protos[type].func_bare_arithmetic) == NULL)
		return 0;
	
	return func_arith(data, operator, operand);
} // }}}

size_type            data_size_type         (data_type type){ // {{{
	if((unsigned)type >= data_protos_size)
		return 0;
	
	return data_protos[type].size_type;
} // }}}

/* vim: set filetype=c: */
