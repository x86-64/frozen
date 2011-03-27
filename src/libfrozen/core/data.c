/**
 * @file data.c
 * @brief Data manipulations
 */
#define DATA_C
#include <libfrozen.h>

#define DATA_FUNC(_type,_name) ( data_protos[_type]->func_##_name ? data_protos[_type]->func_##_name : data_def_##_name )
#define DATA_FUNC_CALL(_type,_name,...) {             \
	f_data_##_name func = DATA_FUNC(_type,_name); \
	return func(__VA_ARGS__);                     \
}

extern data_proto_t * data_protos[];
extern size_t         data_protos_size;

static ssize_t       data_def_read          (data_t *data, data_ctx_t *context, off_t offset, void **buffer, size_t *buffer_size){ // {{{
	ssize_t      ret;
	DT_OFFT      d_offset = 0;
	DT_SIZET     d_size   = data->data_size;
	
	// TODO SECURITY

	hash_data_copy(ret, TYPE_OFFT,  d_offset,  context, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, d_size,    context, HK(size));
	
	if(d_offset > data->data_size || d_size > data->data_size || d_offset + d_size > data->data_size)
		return -EINVAL;  // invalid context parameters
	
	if(offset > d_size)
		return -EINVAL;  // invalid request
	
	d_offset += offset;
	d_size   -= offset;
	
	*buffer_size = d_size = MIN(*buffer_size, d_size);
	
	if(d_size == 0)
		return -1;       // EOF
	
	memcpy(*buffer, data->data_ptr + d_offset, d_size);
	return d_size;
} // }}}
static ssize_t       data_def_write         (data_t *data, data_ctx_t *context, off_t offset, void *buffer, size_t size){ // {{{
	ssize_t      ret;
	DT_OFFT      d_offset = 0;
	DT_SIZET     d_size   = data->data_size;
	
	hash_data_copy(ret, TYPE_OFFT,  d_offset,  context, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, d_size,    context, HK(size));
	
	if(d_offset > data->data_size || d_size > data->data_size || d_offset + d_size > data->data_size)
		return -EINVAL;  // invalid context parameters
	
	if(offset > d_size)
		return -EINVAL;  // invalid request
	
	d_offset += offset;
	d_size   -= offset;
	
	d_size    = MIN(size, d_size);
	
	if(d_size == 0)
		return 0;
	
	memcpy(data->data_ptr + d_offset, buffer, d_size);
	return d_size;
} // }}}
static size_t        data_def_len           (data_t *data, data_ctx_t *context){ // {{{
	return data->data_size;
} // }}}
static size_t        data_def_len2raw       (size_t unitsize){ // {{{
	return unitsize;
} // }}}
static size_t        data_def_rawlen        (data_t *data, data_ctx_t *context){ // {{{
	if(data_protos[data->type]->size_type == SIZE_FIXED)
		return data_protos[data->type]->fixed_size;
	
	return data->data_size;
} // }}}
static ssize_t       data_def_convert       (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){ // {{{
	return -ENOSYS;
} // }}}

/** @brief Read from data to buffer
 *  @param[out]  dst       Data structure
 *  @param[in]   dst_ctx   Data context
 *  @param[in]   offset    Offset to start from
 *  @param       buffer    Pointer to buffer pointer
 *  @param       size      Pointer to size of buffer
 *  @return number of bytes read on success
 *  @return -1 on EOF
 *  @return -EINVAL on error
 */
ssize_t              data_read              (data_t *src, data_ctx_t *src_ctx, off_t offset, void *buffer, size_t size){ // {{{
	ssize_t         rret, ret;
	void           *temp;
	size_t          temp_size, read_size;
	f_data_read     func_read;
	
	if(src == NULL || !data_validate(src))
		return -EINVAL;
	
	func_read  = DATA_FUNC(src->type, read);
	ret        = 0;
	
	while(size > 0){
		temp      = buffer;
		temp_size = size;
		
		rret = func_read(src, src_ctx, offset, &temp, &temp_size);
		if(rret == -1) // EOF
			break;
		if(rret < 0)
			return rret;
		
		read_size = MIN(size, temp_size);
		if(temp != buffer){
			memcpy(buffer, temp, read_size);
		}
		
		buffer   += read_size;
		size     -= read_size;
		
		offset   += read_size;
		ret      += read_size;
	}
	return ret;
} // }}}

static ssize_t       data_read_raw          (data_t *data, data_ctx_t *data_ctx, off_t offset, void **buffer, size_t *size){ // {{{
	if(!data_validate(data))
		return -EINVAL;
	
	DATA_FUNC_CALL(data->type, read,
		data, data_ctx, offset, buffer, size
	);
} // }}}

/** @brief Write to data from buffer
 *  @param[out]  dst       Data structure
 *  @param[in]   dst_ctx   Data context
 *  @param[in]   offset    Offset to start from
 *  @param[in]   buffer    Memory pointer
 *  @param[in]   size      Size of buffer
 *  @return number of bytes written on success
 *  @return -EINVAL on error
 */
ssize_t              data_write             (data_t *data, data_ctx_t *data_ctx, off_t offset, void  *buffer, size_t  size){ // {{{
	if(!data_validate(data))
		return -EINVAL;
	
	DATA_FUNC_CALL(data->type, write,
		data, data_ctx, offset, buffer, size
	);
} // }}}

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

ssize_t              data_type_validate     (data_type type){ // {{{
	if(type != TYPE_INVALID && (unsigned)type < data_protos_size){
		return 1;
	}
	return 0;
} // }}}
ssize_t              data_validate(data_t *data){ // {{{
	if(data == NULL || data->type == TYPE_INVALID || (unsigned)data->type >= data_protos_size)
		return 0;
	
	return 1;
} // }}}
data_type            data_type_from_string  (char *string){ // {{{
	uintmax_t     i;
	data_proto_t *proto;
	
	for(i=0; i<data_protos_size; i++){
		if( (proto = data_protos[i]) == NULL)
			continue;
		
		if(strcasecmp(proto->type_str, string) == 0)
			return proto->type;
	}
	
	return TYPE_INVALID;
} // }}}
char *               data_string_from_type  (data_type type){ // {{{
	if(!data_type_validate(type))
		return "INVALID";
	
	return data_protos[type]->type_str;
} // }}}
data_proto_t *       data_proto_from_type   (data_type type){ // {{{
	if(type == TYPE_INVALID || (unsigned)type >= data_protos_size)
		return NULL;
	
	return data_protos[type];
} // }}}

/** @brief Calculate length of data
 *  @param[in] data       data_t structure
 *  @param[in] data_ctx   Data context
 *  @return length of data
 *  @return 0 on error or zero data length
 */
size_t               data_len               (data_t *data, data_ctx_t *data_ctx){ // {{{
	if(!data_validate(data))
		return 0;
	
	DATA_FUNC_CALL(data->type, len,
		data, data_ctx
	);
} // }}}
size_t               data_rawlen            (data_t *data, data_ctx_t *data_ctx){ // {{{
	if(!data_validate(data))
		return 0;
	
	DATA_FUNC_CALL(data->type, rawlen,
		data, data_ctx
	);
} // }}}
size_t               data_len2raw           (data_type type, size_t unitsize){ // {{{
	if(!data_type_validate(type))
		return 0;
	
	if(data_protos[type]->size_type == SIZE_FIXED)
		return data_protos[type]->fixed_size;
	
	DATA_FUNC_CALL(type, len2raw,
		unitsize
	);
} // }}}

/** @brief Compare data
 *  @param[in] data1      First data to compare
 *  @param[in] data1_ctx  First data context
 *  @param[in] data2      Second data to compare
 *  @param[in] data2_ctx  Second data context
 *  @return -EINVAL on error
 *  @return -EFAULT on unsupported action
 *  @return -ENOSYS if data not support converting
 *  @return 0 or 1 or -1 as result of compare
 */
int                  data_cmp               (data_t *data1, data_ctx_t *data1_ctx, data_t *data2, data_ctx_t *data2_ctx){ // {{{
	char            buffer1_local[DEF_BUFFER_SIZE], buffer2_local[DEF_BUFFER_SIZE];
	void           *buffer1, *buffer2;
	size_t          buffer1_size, buffer2_size, cmp_size;
	ssize_t         ret;
	off_t           offset1, offset2, goffset1, goffset2;
	f_data_cmp      func_cmp;
	
	if(!data_validate(data1) || !data_validate(data2))
		return -EINVAL;
	
	if(data1->type != data2->type){
		// TODO call convert
		
		goffset1     = goffset2     = 0;
		buffer1_size = buffer2_size = 0;
		do {
			if(buffer1_size == 0){
				buffer1      = buffer1_local;
				buffer1_size = DEF_BUFFER_SIZE;
				
				if( (ret = data_read_raw  (data1, data1_ctx, goffset1, &buffer1, &buffer1_size)) < -1)
					return -EINVAL;
				
				if(ret == -1 && buffer2_size != 0)
					return 1;
				
				goffset1 += buffer1_size;
				offset1 = 0;
			}
			if(buffer2_size == 0){
				buffer2      = buffer2_local;
				buffer2_size = DEF_BUFFER_SIZE;
				
				if( (ret = data_read_raw  (data2, data2_ctx, goffset2, &buffer2, &buffer2_size)) < -1)
					return -EINVAL;
				
				if(ret == -1)
					return (buffer1_size == 0) ? 0 : -1;
				
				goffset2 += buffer2_size;
				offset2   = 0;
			}
			
			cmp_size = (buffer1_size < buffer2_size) ? buffer1_size : buffer2_size;
			
			ret = memcmp(buffer1 + offset1, buffer2 + offset2, cmp_size);
			if(ret != 0)
				return ret;
			
			offset1      += cmp_size;
			offset2      += cmp_size;
			buffer1_size -= cmp_size;
			buffer2_size -= cmp_size;
		}while(1);
	}
	
	if( (func_cmp = data_protos[data1->type]->func_cmp) == NULL)
		return -ENOSYS;
	
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
	
	if(!data_validate(operand1) || !data_validate(operand2))
		return -EINVAL;
	
	if(operand1->type != operand2->type){
		// TODO call convert
		return -EFAULT;
	}
	
	if( (func_arith = data_protos[operand1->type]->func_arithmetic) == NULL)
		return -EFAULT;
	
	return func_arith(operator, operand1, operand1_ctx, operand2, operand2_ctx);
} // }}}

/** @brief Convert data from one type to another
 *  @param[in] type       Dsetination data type
 *  @param[in] dst        Destination data
 *  @param[in] dst_ctx    Destination data context
 *  @param[in] src        Source data
 *  @param[in] src_ctx    Source data context
 *  @return 0 on success
 *  @return -EINVAL on invalid input data
 *  @return -ENOSYS if data not support converting
 *  @post Free dst structure with data_free to avoid memory leak
 */
ssize_t              data_convert           (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){ // {{{
	if(!data_validate(dst) || !data_validate(src))
		return -EINVAL;
	
	DATA_FUNC_CALL(dst->type, convert,
		dst, dst_ctx, src, src_ctx
	);
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
ssize_t              data_transfer          (data_t *dst, data_ctx_t *dst_ctx, data_t *src, data_ctx_t *src_ctx){ // {{{
	char            buffer_local[DEF_BUFFER_SIZE];
	void           *buffer;
	size_t          buffer_size;
	ssize_t         rret, wret, transferred;
	off_t           offset;
	
	if(!data_validate(dst) || !data_validate(src))
		return -EINVAL;
	
	offset = transferred = 0;
	do {
		buffer      = buffer_local;
		buffer_size = DEF_BUFFER_SIZE;
		
		if( (rret = data_read_raw (src, src_ctx, offset, &buffer, &buffer_size)) < -1)
			return rret;
		
		if(rret == -1) // EOF from read side
			break;
		
		if( (wret = data_write (dst, dst_ctx, offset,  buffer,  buffer_size)) < 0)
			return wret;
		
		transferred += wret;
		offset      += rret;
		
		if(rret != wret) // EOF from write side
			break;
	}while(1);
	
	return transferred;
} // }}}

/** @brief Allocate memory for data and fill structure
 *  @param[out]  dst    Destination data
 *  @param[in]   type   Data type
 *  @param[in]   size   Buffer size
 *  @return 0 on success
 *  @return -ENOMEM on memory exhaust
 *  @return -EINVAL on invalid input data
 *  @post Free structure with data_free to avoid memory leak
 */
ssize_t              data_alloc             (data_t *dst, data_type type, size_t size){ // {{{
	// TODO IMPORTANT data related allocs
	
	if(dst == NULL)
		return -EINVAL;
	
	dst->type      = type;
	dst->data_size = size;
	
	if(size == 0){ // no zero length memory chunks
		dst->data_ptr = NULL;
		return 0;
	}
	
	if( (dst->data_ptr = malloc(size)) == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}

/** @brief Copy data structure and data
 *  @param[out]  dst    Destination data
 *  @param[in]   src    Source data
 *  @return 0 on success
 *  @return -EFAULT on error
 *  @return -EINVAL on invalid input data
 *  @post Free structure with data_free to avoid memory leak
 */
ssize_t             data_copy                (data_t *dst, data_t *src){ // {{{
	void *data_ptr; // to support (dst eq src)
	
	if(dst == NULL || !data_validate(src))
		return -EINVAL;
	
	data_ptr       = src->data_ptr;
	dst->type      = src->type;
	dst->data_size = src->data_size;
	if( (dst->data_ptr = malloc(src->data_size)) == NULL)
		return -EFAULT;
	
	memcpy(dst->data_ptr, data_ptr, src->data_size);
	return 0;
} // }}}

/** @brief Free data structure
 *  @param[in]  data  Data structure
 */
void                data_free                (data_t *data){ // {{{
	f_data_free  func_free;
	
	if(data == NULL || data->data_ptr == NULL)
		return;
	
	if( (func_free = data_protos[data->type]->func_free) != NULL){
		func_free(data);
		return;
	}
	
	free(data->data_ptr);
	data->data_ptr  = NULL;
	data->data_size = 0;
} // }}}

/* vim: set filetype=c: */
