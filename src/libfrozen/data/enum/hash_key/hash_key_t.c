#include <libfrozen.h>
#include <dataproto.h>
#include <hash_key_t.h>

#include <hashkeys_int.h>
#include <enum/format/format_t.h>

static int hash_bsearch_string(const void *m1, const void *m2){ // {{{
	hash_keypair_t *mi1 = (hash_keypair_t *) m1;
	hash_keypair_t *mi2 = (hash_keypair_t *) m2;
	return strcmp(mi1->key_str, mi2->key_str);
} // }}}
static int hash_bsearch_int(const void *m1, const void *m2){ // {{{
	hash_keypair_t *mi1 = (hash_keypair_t *) m1;
	hash_keypair_t *mi2 = (hash_keypair_t *) m2;
	return (mi1->key_val - mi2->key_val);
} // }}}

static ssize_t data_hash_key_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	hash_keypair_t         key, *ret;
	hash_key_t             key_val                  = 0;

	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(hash_key_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->format){
		case FORMAT(human):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, &buffer, sizeof(buffer) - 1 };
			if(data_query(fargs->src, &r_read) != 0)
				return -EFAULT;
			
			if( (key.key_str = buffer) == NULL)
				return -EINVAL;
			
			/*
			// get valid ordering
			qsort(hash_keys, hash_keys_nelements, hash_keys_size, &hash_bsearch_string);
			int i;
			for(i=0; i<hash_keys_nelements; i++){
				printf("REGISTER_KEY(%s)\n", hash_keys[i].key_str);
			}
			exit(0);
			*/

			if((ret = bsearch(&key, hash_keys,
				hash_keys_nelements, hash_keys_size,
				&hash_bsearch_string)) != NULL
			)
				key_val = ret->key_val;
			
			*(hash_key_t *)(dst->ptr) = key_val;
			return 0;
		
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_hash_key_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	char                  *string;
	hash_keypair_t         key, *ret;

	if(fargs->dest == NULL)
		return -EINVAL;
	
	switch(fargs->dest->type){
		case TYPE_STRINGT:	
			key.key_val = *(hash_key_t *)(src->ptr);
			string      = "(unknown)";

			if(key.key_val != 0){
				if((ret = bsearch(&key, hash_keys,
					hash_keys_nelements, hash_keys_size,
					&hash_bsearch_int)) != NULL
				)
					string = ret->key_str;
			}else{
				string = "(null)";
			}
			fargs->dest->ptr = strdup(string);
			return 0;
		default:
			break;
	};
	return -ENOSYS;
} // }}}		
static ssize_t data_hash_key_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = sizeof(hash_key_t);
	return 0;
} // }}}

data_proto_t hash_key_t_proto = {
	.type                   = TYPE_HASHKEYT,
	.type_str               = "hash_key_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_hash_key_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_hash_key_t_convert_from,
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_hash_key_t_len,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_hash_key_t_len,
	}
};
