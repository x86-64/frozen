#include <libfrozen.h>
#include <hash_key_t.h>
#include <hashkeys_int.h>

static int hash_bsearch_string(const void *m1, const void *m2){ // {{{
	hash_keypair_t *mi1 = (hash_keypair_t *) m1;
	hash_keypair_t *mi2 = (hash_keypair_t *) m2;
	return strcmp(mi1->key_str, mi2->key_str);
} // }}}

static ssize_t data_hash_key_t_convert(data_t *dst, fastcall_convert *fargs){ // {{{
	hash_keypair_t  key, *ret;
	hash_key_t      key_val                  = 0;

	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(hash_key_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->src->type){
		case TYPE_STRINGT:
			if( (key.key_str = (char *)fargs->src->ptr) == NULL)
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
static ssize_t data_hash_key_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = sizeof(hash_key_t);
	return 0;
} // }}}

data_proto_t hash_key_t_proto = {
	.type                   = TYPE_HASHKEYT,
	.type_str               = "hash_key_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT]     = (f_data_func)&data_hash_key_t_convert,
		[ACTION_PHYSICALLEN] = (f_data_func)&data_hash_key_t_len,
		[ACTION_LOGICALLEN]  = (f_data_func)&data_hash_key_t_len,
	}
};
