#include <libfrozen.h>
#include <datatype_t.h>
#include <enum/format/format_t.h>
#include <core/void/void_t.h>

#define DATATYPE_PORTABLE uint32_t

datatype_t     datatype_t_getid_byname(char *name, ssize_t *pret){ // {{{
	uintmax_t              i;
	ssize_t                ret               = -EINVAL;
	datatype_t             type              = TYPE_VOIDT;
	data_proto_t          *proto;
	
	for(i=0; i<data_protos_nitems; i++){
		if( (proto = data_protos[i]) == NULL)
			continue;
		
		if(strcasecmp(proto->type_str, name) == 0){
			type = i;
			ret  = 0;
			break;
		}
	}
	if(pret)
		*pret = ret;
	return type;
} // }}}
datatype_t     datatype_t_getid_byport(DATATYPE_PORTABLE port, ssize_t *pret){ // {{{
	uintmax_t              i;
	ssize_t                ret               = -EINVAL;
	datatype_t             type              = TYPE_VOIDT;
	data_proto_t          *proto;
	
	for(i=0; i<data_protos_nitems; i++){
		if( (proto = data_protos[i]) == NULL)
			continue;
		
		if(proto->type_port == port){
			type = i;
			ret  = 0;
			break;
		}
	}
	if(pret)
		*pret = ret;
	return type;
} // }}}

static ssize_t data_datatype_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              transfered        = 0;
	char                   buffer[DEF_BUFFER_SIZE];
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(datatype_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(human):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, buffer, sizeof(buffer) - 1 };
			if( ( ret = data_query(fargs->src, &r_read)) < 0)
				return ret;
			
			buffer[r_read.buffer_size] = '\0';
			
			*(datatype_t *)(dst->ptr) = datatype_t_getid_byname(buffer, &ret);
			transfered = strlen(buffer);
			break;

		case FORMAT(native):;
			fastcall_read r_clean = { { 5, ACTION_READ }, 0, dst->ptr, sizeof(datatype_t) };
			if( (ret = data_query(fargs->src, &r_clean)) < 0)
				return ret;
			
			transfered = sizeof(datatype_t);
			break;
		
		case FORMAT(packed):;
			DATATYPE_PORTABLE type_port = 0;

			fastcall_read r_binary = { { 5, ACTION_READ }, 0, &type_port, sizeof(type_port) };
			if( (ret = data_query(fargs->src, &r_binary)) < 0)
				return ret;
			
			*(datatype_t *)(dst->ptr) = datatype_t_getid_byport(type_port, &ret);
			transfered = sizeof(type_port);
			break;
			
		default:
			return -ENOSYS;
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_datatype_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	datatype_t             type;
	char                  *string            = "INVALID";
	uintmax_t              transfered;
	
	if(src->ptr == NULL)
		return -EINVAL;
	
	type = *(datatype_t *)(src->ptr);
	
	switch(fargs->format){
		case FORMAT(config):
		case FORMAT(human):
			if((unsigned)type < data_protos_nitems){
				string = data_protos[type]->type_str;
			}
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, string, strlen(string) };
			ret        = data_query(fargs->dest, &r_write);
			transfered = r_write.buffer_size;
			break;
			
		case FORMAT(native):;
			fastcall_write r_clean = { { 5, ACTION_WRITE }, 0, &type, sizeof(type) };
			ret        = data_query(fargs->dest, &r_clean);
			transfered = r_clean.buffer_size;
			break;
			
		case FORMAT(packed):;
			if((unsigned)type > data_protos_nitems)
				return -EINVAL;
			
			DATATYPE_PORTABLE  type_port = data_protos[type]->type_port;
			
			fastcall_write r_binary = { { 5, ACTION_WRITE }, 0, &type_port, sizeof(type_port) };
			ret        = data_query(fargs->dest, &r_binary);
			transfered = r_binary.buffer_size;
			break;
			
		default:
			return -ENOSYS;
	}
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}		
static ssize_t data_datatype_t_len(data_t *data, fastcall_length *fargs){ // {{{
	switch(fargs->format){
		case FORMAT(native):  fargs->length = sizeof(datatype_t);        break;
		case FORMAT(packed): fargs->length = sizeof(DATATYPE_PORTABLE); break;
		case FORMAT(config):
		case FORMAT(human):
			// TODO convert and measure
		default:
			return -ENOSYS;
	}
	return 0;
} // }}}

data_proto_t datatype_t_proto = {
	.type                   = TYPE_DATATYPET,
	.type_str               = "datatype_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_datatype_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_datatype_t_convert_from,
		[ACTION_LENGTH]       = (f_data_func)&data_datatype_t_len,
	}
};
