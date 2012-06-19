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

ssize_t              helper_data_convert_from(data_t *data, data_t *src, uintmax_t format, uintmax_t *transfered){ // {{{
	ssize_t               ret;
	fastcall_convert_from request = { { 4, ACTION_CONVERT_FROM }, src, format };
	ret = data_query(data, &request);
	if(transfered)
		*transfered = request.transfered;
	return ret;
} // }}}
ssize_t              helper_data_convert_to(data_t *data, data_t *dest, uintmax_t format, uintmax_t *transfered){ // {{{
	ssize_t             ret;
	fastcall_convert_to request = { { 5, ACTION_CONVERT_TO }, dest, format };
	ret = data_query(data, &request);
	if(transfered)
		*transfered = request.transfered;
	return ret;
} // }}}
ssize_t              helper_data_free(data_t *data){ // {{{
	fastcall_free request = { { 2, ACTION_FREE } };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_control(data_t *data, uintmax_t function, data_t *key, data_t *value){ // {{{
	fastcall_control request = { { 5, ACTION_CONTROL }, function, key, value };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_consume(data_t *data, data_t *dest){ // {{{
	fastcall_consume request = { { 3, ACTION_CONSUME }, dest };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_resize(data_t *data, uintmax_t length){ // {{{
	fastcall_resize request = { { 3, ACTION_RESIZE }, length };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_length(data_t *data, uintmax_t *length, uintmax_t format){ // {{{
	ssize_t         ret;
	fastcall_length request = { { 4, ACTION_LENGTH }, 0, format };
	ret = data_query(data, &request);
	if(length)
		*length = request.length;
	return ret;
} // }}}
ssize_t              helper_data_read(data_t *data, uintmax_t offset, void *buffer, uintmax_t *buffer_size){ // {{{
	ssize_t       ret;
	fastcall_read request = { { 5, ACTION_READ }, offset, buffer, buffer_size ? *buffer_size : 0 };
	ret = data_query(data, &request);
	if(buffer_size)
		*buffer_size = request.buffer_size;
	return ret;
} // }}}
ssize_t              helper_data_write(data_t *data, uintmax_t offset, void *buffer, uintmax_t *buffer_size){ // {{{
	ssize_t       ret;
	fastcall_write request = { { 5, ACTION_WRITE }, offset, buffer, buffer_size ? *buffer_size : 0 };
	ret = data_query(data, &request);
	if(buffer_size)
		*buffer_size = request.buffer_size;
	return ret;
} // }}}
ssize_t              helper_data_add(data_t *data, data_t *data2){ // {{{
	fastcall_add request = { { 3, ACTION_ADD }, data2 };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_sub(data_t *data, data_t *data2){ // {{{
	fastcall_sub request = { { 3, ACTION_SUB }, data2 };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_mul(data_t *data, data_t *data2){ // {{{
	fastcall_mul request = { { 3, ACTION_MULTIPLY }, data2 };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_div(data_t *data, data_t *data2){ // {{{
	fastcall_div request = { { 3, ACTION_DIVIDE }, data2 };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_inc(data_t *data){ // {{{
	fastcall_increment request = { { 2, ACTION_INCREMENT } };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_dec(data_t *data){ // {{{
	fastcall_decrement request = { { 2, ACTION_DECREMENT } };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_compare(data_t *data, data_t *data2, uintmax_t *result){ // {{{
	ssize_t          ret;
	fastcall_compare request = { { 4, ACTION_COMPARE }, data2 };
	ret = data_query(data, &request);
	if(result)
		*result = request.result;
	return ret;
} // }}}
ssize_t              helper_data_create(data_t *data, data_t *key, data_t *value){ // {{{
	fastcall_create request = { { 4, ACTION_CREATE }, key, value };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_lookup(data_t *data, data_t *key, data_t *value){ // {{{
	fastcall_lookup request = { { 4, ACTION_LOOKUP }, key, value };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_update(data_t *data, data_t *key, data_t *value){ // {{{
	fastcall_update request = { { 4, ACTION_UPDATE }, key, value };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_delete(data_t *data, data_t *key, data_t *value){ // {{{
	fastcall_delete request = { { 4, ACTION_DELETE }, key, value };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_push(data_t *data, data_t *value){ // {{{
	fastcall_push request = { { 4, ACTION_PUSH }, value };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_pop(data_t *data, data_t *value){ // {{{
	fastcall_pop request = { { 4, ACTION_POP }, value };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_enum(data_t *data, data_t *dest){ // {{{
	fastcall_enum request = { { 3, ACTION_ENUM }, dest };
	return data_query(data, &request);
} // }}}
ssize_t              helper_data_view(data_t *data, uintmax_t format, void **ptr, uintmax_t *length, data_t *freeit){ // {{{
	ssize_t       ret;
	fastcall_view request = { { 6, ACTION_VIEW }, format };
	ret = data_query(data, &request);
	if(ptr)
		*ptr    = request.ptr;
	if(length)
		*length = request.length;
	if(freeit)
		*freeit = request.freeit;
	return ret;
} // }}}

data_t               helper_data_from_string(datatype_t type, char *string){ // {{{
	data_t         new_data   = { type, NULL };
	data_t         d_string   = DATA_STRING(string);
	
	fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, &d_string, FORMAT(config) };
	if(data_query(&new_data, &r_convert) < 0){
		data_set_void(&new_data);
	}
	return new_data;
} // }}}
data_t               helper_data_from_hash(datatype_t type, hash_t *hash){ // {{{
	data_t         new_data   = { type, NULL };
	data_t         d_config   = DATA_PTR_HASHT(hash);
	
	fastcall_convert_from r_convert = { { 4, ACTION_CONVERT_FROM }, &d_config, FORMAT(hash) };
	if(data_query(&new_data, &r_convert) < 0){
		data_set_void(&new_data);
	}
	return new_data;
} // }}}

