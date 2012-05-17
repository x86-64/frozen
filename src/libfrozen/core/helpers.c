#include <libfrozen.h>

ssize_t              helper_key_current     (data_t *key, data_t *token){ // {{{
	data_t          d_key    = DATA_UINTT(0);
	fastcall_lookup r_lookup = { { 4, ACTION_LOOKUP }, &d_key, token };
	return data_query(key, &r_lookup);
} // }}}

ssize_t              helper_control_data    (data_t *data, fastcall_control *fargs, helper_control_data_t *internal_ptrs){ // {{{
	ssize_t                ret;
	hashkey_t              hk_key;
	data_t                 d_key;
	data_t                *d_key_ptr         = &d_key;
	
	if(fargs->key == NULL || helper_key_current(fargs->key, &d_key) < 0){
		// empty key
		fargs->value = data;
		return 0;
	}
	data_get(ret, TYPE_HASHKEYT, hk_key, d_key_ptr);
	if(ret != 0){
		ret = -EINVAL;
		goto exit;
	}
	
	while(internal_ptrs->key != 0){
		if(internal_ptrs->key == hk_key){
			data_t           sl_key    = DATA_SLICET(fargs->key, 1, ~0);
			fastcall_control r_control = { { 5, ACTION_CONTROL }, fargs->function, &sl_key, fargs->value };
			ret          = data_query(internal_ptrs->data, &r_control);
			fargs->value = r_control.value;
			goto exit;
		}
		internal_ptrs++;
	}
	ret = -ENOSYS;
exit:
	data_free(&d_key);
	return ret;
} // }}}

