#define ACTION_C
#include <libfrozen.h>
#include <action_t.h>
#include <enum/format/format_t.h>

#define EMODULE 51

static ssize_t data_action_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	keypair_t             *kp;
	char                   buffer[DEF_BUFFER_SIZE];
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(action_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(human):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read) != 0)
				return -EFAULT;
			
			buffer[r_read.buffer_size] = '\0';

			for(kp = &actions[0]; kp->key_str != NULL; kp++){
				if(strcasecmp(kp->key_str, buffer) == 0){
					*(action_t *)(dst->ptr) = kp->key_val;
					return 0;
				}
			}
			return -EINVAL;
		case FORMAT(binary):;
			fastcall_read r_read2 = { { 5, ACTION_READ }, 0, dst->ptr, sizeof(action_t) };
			return data_query(fargs->src, &r_read2);
		
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_action_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	action_t               value;
	uintmax_t              transfered;
	keypair_t             *kp;
	char                  *string            = "(unknown)";
	
	if(fargs->dest == NULL || src->ptr == NULL)
		return -EINVAL;
	
	value = *(action_t *)src->ptr;
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(clean):;
		case FORMAT(human):;
			// find in static keys first
			for(kp = &actions[0]; kp->key_str != NULL; kp++){
				if(kp->key_val == value){
					string = kp->key_str;
					goto found;
				}
			}
		
		found:;
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, string, strlen(string) };
			ret        = data_query(fargs->dest, &r_write);
			transfered = r_write.buffer_size;
			break;
		
		case FORMAT(binary):;
			fastcall_write r_write2 = { { 5, ACTION_WRITE }, 0, &value, sizeof(value) };
			ret        = data_query(fargs->dest, &r_write2);
			transfered = r_write.buffer_size;
			break;
			
		default:
			return -ENOSYS;
	}
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}		
static ssize_t data_action_t_len(data_t *data, fastcall_length *fargs){ // {{{
	switch(fargs->format){
		case FORMAT(binary):
		case FORMAT(clean):
			fargs->length = sizeof(action_t);
			return 0;
		default:
			break;
	}
	return -ENOSYS;
} // }}}

data_proto_t action_t_proto = {
	.type_str               = "action_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_action_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_action_t_convert_from,
		[ACTION_LENGTH]       = (f_data_func)&data_action_t_len,
	}
};
