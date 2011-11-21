#define FORMAT_C
#include <libfrozen.h>
#include <dataproto.h>
#include <format_t.h>

static int hash_bsearch_string(const void *m1, const void *m2){ // {{{
	keypair_t *mi1 = (keypair_t *) m1;
	keypair_t *mi2 = (keypair_t *) m2;
	return strcmp(mi1->key_str, mi2->key_str);
} // }}}
static int hash_bsearch_int(const void *m1, const void *m2){ // {{{
	keypair_t *mi1 = (keypair_t *) m1;
	keypair_t *mi2 = (keypair_t *) m2;
	return (mi1->key_val - mi2->key_val);
} // }}}

static ssize_t data_format_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	char                   buffer[DEF_BUFFER_SIZE] = { 0 };
	keypair_t           key, *ret;
	format_t               key_val                  = 0;

	if(fargs->src == NULL)
		return -EINVAL;
	
	if(dst->ptr == NULL){ // no buffer, alloc new
		if( (dst->ptr = malloc(sizeof(format_t))) == NULL)
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
			qsort(formats, formats_nelements, formats_size, &hash_bsearch_string);
			int i;
			for(i=0; i<formats_nelements; i++){
				printf("REGISTER_KEY(%s)\n", formats[i].key_str);
			}
			exit(0);
			*/

			if((ret = bsearch(&key, formats,
				formats_nelements, formats_size,
				&hash_bsearch_string)) != NULL
			)
				key_val = ret->key_val;
			
			*(format_t *)(dst->ptr) = key_val;
			return 0;
		
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_format_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	char                  *string;
	keypair_t         key, *ret;

	if(fargs->dest == NULL)
		return -EINVAL;
	
	switch(fargs->dest->type){
		case TYPE_STRINGT:	
			key.key_val = *(format_t *)(src->ptr);
			string      = "(unknown)";

			if(key.key_val != 0){
				if((ret = bsearch(&key, formats,
					formats_nelements, formats_size,
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
static ssize_t data_format_t_len(data_t *data, fastcall_len *fargs){ // {{{
	fargs->length = sizeof(format_t);
	return 0;
} // }}}

data_proto_t format_t_proto = {
	.type                   = TYPE_HASHKEYT,
	.type_str               = "format_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_format_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_format_t_convert_from,
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_format_t_len,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_format_t_len,
	}
};
