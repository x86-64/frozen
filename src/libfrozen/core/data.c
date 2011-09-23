/**
 * @file data.c
 * @brief Data manipulations
 */
#define DATA_C
#include <libfrozen.h>

extern data_proto_t * data_protos[];
extern size_t         data_protos_size;

ssize_t              data_validate(data_t *data){ // {{{
	if(data == NULL || data->type == TYPE_INVALID || (unsigned)data->type >= data_protos_size)
		return 0;
	
	return 1;
} // }}}
ssize_t              data_type_validate     (data_type type){ // {{{
	if(type != TYPE_INVALID && (unsigned)type < data_protos_size){
		return 1;
	}
	return 0;
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

ssize_t              data_query         (data_t *data, void *args){ // {{{
	f_data_func            func;
	data_proto_t          *proto;
	fastcall_header       *fargs             = (fastcall_header *)args;
	
	if(
		data  == NULL || data->type == TYPE_INVALID || (unsigned)data->type >= data_protos_size ||
		fargs == NULL || fargs->nargs < 2
	)
		return -EINVAL;
	
	proto = data_protos[data->type];
	
	switch(proto->api_type){
		case API_DEFAULT_HANDLER: func = proto->handler_default;           break;
		case API_HANDLERS:        func = proto->handlers[ fargs->action ]; break;
	};
	if( func == NULL && (func = data_protos[TYPE_DEFAULTT]->handlers[ fargs->action ]) == NULL)
		return -ENOSYS;
	
	if( fargs->nargs < fastcall_nargs[ fargs->action ] )
		return -EINVAL;
	
	return func(data, args);
} // }}}

/*
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
*/


/* vim: set filetype=c: */
