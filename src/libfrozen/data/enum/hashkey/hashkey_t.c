#define HASHKEYS_C
#include <libfrozen.h>
#include <hashkey_t.h>

#include <enum/format/format_t.h>

#define ERRORS_MODULE_ID 46
#define ERRORS_MODULE_NAME "enum/hashkey"
#define HASHKEY_PORTABLE uint32_t

#if defined DYNAMIC_KEYS_CHECK || defined RESOLVE_DYNAMIC_KEYS
static list                    dynamic_keys   = LIST_INITIALIZER;
#endif

static ssize_t data_hashkey_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	keypair_t             *kp;
	hashkey_t              key_val;
	
	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(hashkey_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(human):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if( (ret = data_query(fargs->src, &r_read)) < 0)
				return ret;
			
			key_val = portable_hash(buffer);
	
	#ifdef COLLISION_CHECK
		#ifdef STATIC_KEYS_CHECK
			// find collisions
			for(kp = &hashkeys[0]; kp->key_str != NULL; kp++){
				if(kp->key_val == key_val && strcmp(kp->key_str, buffer) != 0)
					goto collision;
			}
		#endif
		#ifdef DYNAMIC_KEYS_CHECK
			void                  *iter              = NULL;
			
			list_rdlock(&dynamic_keys);
			while( (kp = list_iter_next(&dynamic_keys, &iter)) != NULL){
				if(kp->key_val == key_val && strcmp(kp->key_str, buffer) != 0){
					list_unlock(&dynamic_keys);
					goto collision;
				}
			}
			list_unlock(&dynamic_keys);
		#endif
	#endif
		#if defined DYNAMIC_KEYS_CHECK || defined RESOLVE_DYNAMIC_KEYS
			keypair_t             *newkey            = malloc(sizeof(keypair_t));
			
			newkey->key_str = strdup(buffer);
			newkey->key_val = key_val;
			list_add(&dynamic_keys, newkey);
		#endif
	
			*(hashkey_t *)(dst->ptr) = key_val;
			return 0;
		
		case FORMAT(binary):;
			HASHKEY_PORTABLE  key_port = 0;
			
			fastcall_read r_binary = { { 5, ACTION_READ }, 0, &key_port, sizeof(key_port) };
			if( (ret = data_query(fargs->src, &r_binary)) < 0)
				return ret;
			
			*(hashkey_t *)(dst->ptr) = key_port;
			return 0;
			
		default:
			break;
	}
	return -ENOSYS;
	goto collision; // dummy

collision:
	// report collision
	fprintf(stderr, "key collision: %s <=> %s\n", kp->key_str, buffer);
	return error("key collision");
} // }}}
static ssize_t data_hashkey_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	hashkey_t              value;
	uintmax_t              transfered;
	keypair_t             *kp;
	void                  *iter              = NULL;
	char                  *string            = "(unknown)";
	
	if(fargs->dest == NULL || src->ptr == NULL)
		return -EINVAL;
	
	value = *(hashkey_t *)src->ptr;
	
	switch(fargs->format){
		case FORMAT(config):;
		case FORMAT(clean):;
		case FORMAT(human):;
			// find in static keys first
			for(kp = &hashkeys[0]; kp->key_str != NULL; kp++){
				if(kp->key_val == value){
					string = kp->key_str;
					goto found;
				}
			}
			
		#ifdef RESOLVE_DYNAMIC_KEYS
			// find in dynamic keys
			list_rdlock(&dynamic_keys);
			while( (kp = list_iter_next(&dynamic_keys, &iter)) != NULL){
				if(kp->key_val == value){
					string = kp->key_str;
					list_unlock(&dynamic_keys);
					goto found;
				}
			}
			list_unlock(&dynamic_keys);
		#endif
		
		found:;
			fastcall_write r_write = { { 5, ACTION_WRITE }, 0, string, strlen(string) };
			ret        = data_query(fargs->dest, &r_write);
			transfered = r_write.buffer_size;
			break;
		
		case FORMAT(binary):;
			HASHKEY_PORTABLE key_port = (HASHKEY_PORTABLE)value;

			fastcall_write r_binary = { { 5, ACTION_WRITE }, 0, &key_port, sizeof(key_port) };
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
static ssize_t data_hashkey_t_len(data_t *data, fastcall_length *fargs){ // {{{
	switch(fargs->format){
		case FORMAT(binary): fargs->length = sizeof(HASHKEY_PORTABLE); break;
		default:
			return -ENOSYS;
	}
	return 0;
} // }}}

data_proto_t hashkey_t_proto = {
	.type                   = TYPE_HASHKEYT,
	.type_str               = "hashkey_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_hashkey_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_hashkey_t_convert_from,
		[ACTION_LENGTH]       = (f_data_func)&data_hashkey_t_len,
	}
};
