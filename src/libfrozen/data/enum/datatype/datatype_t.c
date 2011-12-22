#include <libfrozen.h>
#include <dataproto.h>
#include <datatype_t.h>
#include <enum/format/format_t.h>

static ssize_t data_datatype_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(datatype_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(human):;
			uintmax_t     i;
			data_proto_t *proto;
			
			for(i=0; i<data_protos_size; i++){
				if( (proto = data_protos[i]) == NULL)
					continue;
				
				if(strcasecmp(proto->type_str, (char *)fargs->src->ptr) == 0){
					*(datatype_t *)(dst->ptr) = proto->type;
					return 0;
				}
			}
			return -EINVAL;

		default:
			// TODO from any type
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_datatype_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	datatype_t             type;
	char                  *string            = "INVALID";
	
	if(src->ptr == NULL)
		return -EINVAL;
	
	type = *(datatype_t *)(src->ptr);
	
	if(type != TYPE_INVALID && (unsigned)type < data_protos_size){
		string = data_protos[type]->type_str;
	}
	
	fastcall_write r_write = { { 5, ACTION_WRITE }, 0, string, strlen(string) };
	ret        = data_query(fargs->dest, &r_write);
	
	if(fargs->header.nargs >= 5)
		fargs->transfered = r_write.buffer_size;
	
	return ret;
} // }}}		
static ssize_t data_datatype_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = sizeof(datatype_t);
	return 0;
} // }}}

data_proto_t datatype_t_proto = {
	.type                   = TYPE_DATATYPET,
	.type_str               = "datatype_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_datatype_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_datatype_t_convert_from,
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_datatype_t_len,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_datatype_t_len,
	}
};
