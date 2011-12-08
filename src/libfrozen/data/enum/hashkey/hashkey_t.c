#define HASHKEYS_C
#include <libfrozen.h>
#include <dataproto.h>
#include <hashkey_t.h>

#include <enum/format/format_t.h>

static ssize_t data_hashkey_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	uintmax_t              i                       = 1;
	char                   c;
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	char                  *p                       = buffer;
	hashkey_t              key_val                 = 0;
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(hashkey_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->format){
		case FORMAT(human):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read) != 0)
				return -EFAULT;
			
			while((c = *p++)){
				key_val += c * i * i;
				i++;
			}
			*(hashkey_t *)(dst->ptr) = key_val;
			return 0;
		
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_hashkey_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	char                  *string;
	//keypair_t              key;

	if(fargs->dest == NULL)
		return -EINVAL;
	
	switch(fargs->dest->type){
		case TYPE_STRINGT:	
			//key.key_val = *(hashkey_t *)(src->ptr);
			string      = "(unknown)";
			
			// TODO
			fargs->dest->ptr = strdup(string);
			return 0;
		default:
			break;
	};
	return -ENOSYS;
} // }}}		
static ssize_t data_hashkey_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = sizeof(hashkey_t);
	return 0;
} // }}}

data_proto_t hashkey_t_proto = {
	.type                   = TYPE_HASHKEYT,
	.type_str               = "hashkey_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_hashkey_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_hashkey_t_convert_from,
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_hashkey_t_len,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_hashkey_t_len,
	}
};
