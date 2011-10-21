/**
 * @file data.c
 * @brief Data manipulations
 */
#define DATA_C
#include <libfrozen.h>

ssize_t              data_validate          (data_t *data){ // {{{
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
*/


/* vim: set filetype=c: */
