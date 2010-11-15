/**
 * @file data.c
 * @brief Data manipulations
 */
#include <libfrozen.h>

#define DEF_BUFFER_SIZE 1024

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

int                  data_convert           (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx);

int                  data_read              (data_t *src, data_ctx_t *src_ctx, void *buffer, size_t size);
int                  data_write             (data_t *dst, data_ctx_t *dst_ctx, void *buffer, size_t size);

/** @brief Get data type
 *  @param[in] data  Data structure
 *  @return data type
 */
inline data_type     data_value_type        (data_t *data){ // {{{
	return data->type;
} // }}}

/** @brief Get data raw pointer
 *  @param[in]  data  Data structure
 *  @return 0
 */
inline void *        data_value_ptr         (data_t *data){ // {{{
	return data->data_ptr;
} // }}}

/** @brief Get raw data len
 *  @param[in]  data Data structure
 *  @return data size
 */
inline size_t        data_value_len         (data_t *data){ // {{{
	return data->data_size;
} // }}}

int                  data_type_is_valid     (data_type type){ // {{{
	if(type != TYPE_INVALID && (unsigned)type < data_protos_size){
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
	
	return TYPE_INVALID;
} // }}}
data_proto_t *       data_proto_from_type   (data_type type){ // {{{
	if((unsigned)type >= data_protos_size)
		return 0;
	
	return &data_protos[type];
} // }}}
size_type            data_size_type         (data_type type){ // {{{
	if((unsigned)type >= data_protos_size)
		return 0;
	
	return data_protos[type].size_type;
} // }}}

/** @brief Create data context
 *  @param[out] ctx      Place to write data ctx
 *  @param[in]  type     Data type
 *  @param[in]  context  Hash with context parameters or NULL
 *  @return 0 on success
 *  @return -EINVAL on invalid type
 */
/*
int                  data_ctx_init          (data_ctx_t *ctx, data_type type, hash_t *context){ // {{{
	f_data_ctx_new   func_ctx_new;
	void            *user_data;
	
	if((unsigned)type >= data_protos_size){
		memset(ctx, 0, sizeof(data_ctx_t));
		return -EINVAL;
	}
	
	user_data = NULL;
	if( (func_ctx_new = data_protos[type].func_ctx_new) != NULL){
		user_data = func_ctx_new(context);
	}
	
	ctx->data_proto     = &(data_protos[type]);
	ctx->user_data      = user_data;
	return 0;
} // }}} */

/** @brief Destory data context
 *  @param[in] ctx Data context
 *  @return 0
 */
/*
int                  data_ctx_destroy       (data_ctx_t *ctx){ // {{{
	if( ctx->data_proto != NULL && ctx->data_proto->func_ctx_free != NULL)
		ctx->data_proto->func_ctx_free(ctx->user_data);
	
	return 0;
} // }}} */

/** @brief Calculate length of data
 *  @param[in] data       data_t structure
 *  @param[in] data_ctx   Data context
 *  @return length of data
 *  @return 0 on error or zero data length
 */
size_t               data_len               (data_t *data, data_ctx_t *data_ctx){ // {{{
	f_data_len   func_len;
	data_type    type;
	
	type = data->type;
	if((unsigned)type >= data_protos_size)
		return 0;
	
	switch(data_protos[type].size_type){
		case SIZE_FIXED:
			return data_protos[type].fixed_size; // TODO respect buffer size, struct.c need rewrite for this
			
		case SIZE_VARIABLE:
			if( (func_len = data_protos[type].func_len) == NULL || data == NULL)
				return 0;
					
			return func_len(data, data_ctx);
	}
	return 0;
} // }}}

/** @brief Compare data
 *  @param[in] data1      First data to compare
 *  @param[in] data1_ctx  First data context
 *  @param[in] data2      Second data to compare
 *  @param[in] data2_ctx  Second data context
 *  @return -EINVAL on error
 *  @return -EFAULT on unsupported action
 *  @return 0 or 1 or -1 as result of compare
 */
int                  data_cmp               (data_t *data1, data_ctx_t *data1_ctx, data_t *data2, data_ctx_t *data2_ctx){ // {{{
	f_data_cmp   func_cmp;
	
	if((unsigned)data1->type >= data_protos_size || (unsigned)data2->type >= data_protos_size)
		return -EINVAL;
	
	if(data1->type != data2->type){
		// TODO call convert
		return -EFAULT;
	}
	
	if( (func_cmp = data_protos[data1->type].func_cmp) == NULL)
		return -EFAULT;
	
	return func_cmp(data1, data1_ctx, data2, data2_ctx);
} // }}}

/** @brief Do some math with data
 *  @param[in]  operator      Action to do, for example '+', '-', '*', '/'
 *  @param[in]  operand1      First operand for operation
 *  @param[in]  operand1_ctx  First operand context
 *  @param[in]  operand2      Second operand for operation
 *  @param[in]  operand2_ctx  Second operand context
 *  @return 0 on success
 *  @return -EINVAL on error
 *  @return -EFAULT on unsupported action
 */
int                  data_arithmetic        (char operator, data_t *operand1, data_ctx_t *operand1_ctx, data_t *operand2, data_ctx_t *operand2_ctx){ // {{{
	f_data_arith  func_arith;
	
	if((unsigned)operand1->type >= data_protos_size || (unsigned)operand2->type >= data_protos_size)
		return -EINVAL;
	
	if(operand1->type != operand2->type){
		// TODO call convert
		return -EFAULT;
	}
	
	if( (func_arith = data_protos[operand1->type].func_arithmetic) == NULL)
		return -EFAULT;
	
	return func_arith(operator, operand1, operand1_ctx, operand2, operand2_ctx);
} // }}}

/** @brief Convert data from one type to another
 *  @param[in] dst        Destination data
 *  @param[in] dst_ctx    Destination data context
 *  @param[in] src        Source data
 *  @param[in] src_ctx    Source data context
 *  @return 0 on success
 */
int                  data_convert           (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){
	return -1;
}

static ssize_t       data_def_read          (data_t *data, data_ctx_t *context, off_t offset, void **buffer, size_t *buffer_size){ // {{{
	off_t    d_offset;
	size_t   d_size;
	hash_t  *temp;
	
	d_offset =
		( (temp = hash_find_typed(context, TYPE_OFFT, "offset")) != NULL) ?
		HVALUE(temp, off_t) : 0;
	d_size =
		( (temp = hash_find_typed(context, TYPE_SIZET, "size")) != NULL) ?
		HVALUE(temp, size_t) : data->data_size;
	
	if(d_offset > data->data_size || d_size > data->data_size || d_offset + d_size > data->data_size)
		return -EINVAL;  // invalid context parameters
	
	if(offset > d_size)
		return -EINVAL;  // invalid request
	
	d_offset += offset;
	d_size   -= offset;
	
	d_size    = MIN(*buffer_size, d_size);
	
	if(d_size == 0)
		return -1;       // EOF
	
	memcpy(*buffer, data->data_ptr + d_offset, d_size);
	return d_size;
} // }}}
static ssize_t       data_def_write         (data_t *data, data_ctx_t *context, off_t offset, void *buffer, size_t size){ // {{{
	off_t    d_offset;
	size_t   d_size;
	hash_t  *temp;
	
	d_offset =
		( (temp = hash_find_typed(context, TYPE_OFFT, "offset")) != NULL) ?
		HVALUE(temp, off_t) : 0;
	d_size =
		( (temp = hash_find_typed(context, TYPE_SIZET, "size")) != NULL) ?
		HVALUE(temp, size_t) : data->data_size;
	
	if(d_offset > data->data_size || d_size > data->data_size || d_offset + d_size > data->data_size)
		return -EINVAL;  // invalid context parameters
	
	if(offset > d_size)
		return -EINVAL;  // invalid request
	
	d_offset += offset;
	d_size   -= offset;
	
	d_size    = MIN(size, d_size);
	
	if(d_size == 0)
		return -EINVAL;  // bad request
	
	memcpy(data->data_ptr + d_offset, buffer, d_size);
	return d_size;
} // }}}

/** @brief Transfer info from one data to another
 *  @param[in]   dst      Destination data
 *  @param[in] dst_ctx    Destination data context
 *  @param[in] src        Source data
 *  @param[in] src_ctx    Source data context
 *  @return number of bytes transferred
 *  @return -EINVAL on error
 *  @return -EFAULT on write error
 */
ssize_t              data_transfer          (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){
	char            buffer_local[DEF_BUFFER_SIZE];
	void           *buffer;
	size_t          buffer_size;
	ssize_t         rret, wret, transferred;
	off_t           offset;
	f_data_read     func_read;
	f_data_write    func_write;
	
	func_read  = data_protos[src->type].func_read;
	func_read  = func_read  ? func_read : data_def_read;
	
	func_write = data_protos[dst->type].func_write;
	func_write = func_write ? func_write : data_def_write;
	
	offset = transferred = 0;
	do {
		buffer      = buffer_local;
		buffer_size = DEF_BUFFER_SIZE;
		
		rret = func_read  (src, src_ctx, offset, &buffer, &buffer_size);
		if(rret == -1) // EOF
			return transferred;
		if(rret < 0) // error
			return -EINVAL;
		
		wret = func_write (dst, dst_ctx, offset, buffer, buffer_size);
		if(wret < 0)
			return -EINVAL;
		if(rret != wret)
			return -EFAULT;
		
		transferred += rret;
		offset      += rret;
	}while(1);
	return -EFAULT;
}

/* vim: set filetype=c: */
