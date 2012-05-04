#define ACTION_C
#include <libfrozen.h>
#include <action_t.h>
#include <enum/format/format_t.h>

#define ERRORS_MODULE_ID 51
#define ERRORS_MODULE_NAME "enum/action"

#define PACKED uintmax_t

static ssize_t action_t_find_value(char **string, action_t value){ // {{{
	keypair_t             *kp;
	
	for(kp = &actions[0]; kp->key_str != NULL; kp++){
		if(kp->key_val == value){
			*string = kp->key_str;
			return 0;
		}
	}
	return -EINVAL;
} // }}}
static ssize_t action_t_find_string(char *string, action_t *value){ // {{{
	keypair_t             *kp;
	
	for(kp = &actions[0]; kp->key_str != NULL; kp++){
		if(strcasecmp(kp->key_str, string) == 0){
			*value = kp->key_val;
			return 0;
		}
	}
	return -EINVAL;
} // }}}
static ssize_t action_t_find_packed(PACKED packed, action_t *value){ // {{{
	keypair_t             *kp;
	
	for(kp = &actions[0]; kp->key_str != NULL; kp++){
		if( portable_hash(kp->key_str) == packed ){
			*value = kp->key_val;
			return 0;
		}
	}
	return -EINVAL;
} // }}}

static ssize_t data_action_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              transfered        = 0;
	char                   buffer[DEF_BUFFER_SIZE];
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(action_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(human):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if( (ret = data_query(fargs->src, &r_read)) < 0)
				return ret;
			
			buffer[r_read.buffer_size] = '\0';
			
			ret        = action_t_find_string(buffer, (action_t *)dst->ptr);
			transfered = r_read.buffer_size;
			break;
			
		case FORMAT(packed):;
			PACKED          packed;
			
			fastcall_read r_read2 = { { 5, ACTION_READ }, 0, &packed, sizeof(packed) };
			if( (ret = data_query(fargs->src, &r_read2)) < 0)
				return ret;
			
			if( (ret = action_t_find_packed(packed, (action_t *)dst->ptr)) < 0)
				return ret;
			
			transfered = r_read2.buffer_size;
			break;
		
		default:
			return -ENOSYS;
	}
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_action_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	action_t               value;
	uintmax_t              transfered;
	char                  *string            = "(unknown)";
	
	if(fargs->dest == NULL || src->ptr == NULL)
		return -EINVAL;
	
	value = *(action_t *)src->ptr;
	
	if( (ret = action_t_find_value(&string, value)) < 0)
		return ret;
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(native):;
		case FORMAT(human):;
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, string, strlen(string) };
			ret        = data_query(fargs->dest, &r_write);
			transfered = r_write.buffer_size;
			break;
		
		case FORMAT(packed):;
			PACKED        packed = portable_hash(string);
			
			fastcall_write r_write2 = { { 5, ACTION_WRITE }, 0, &packed, sizeof(packed) };
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
		case FORMAT(native):
			fargs->length = sizeof(action_t);
			return 0;
		case FORMAT(packed):
			fargs->length = sizeof(PACKED);
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
