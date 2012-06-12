#include <libfrozen.h>
#include <complexkey_t.h>

#include <core/void/void_t.h>
#include <core/hash/hash_t.h>
#include <storage/raw/raw_t.h>
#include <numeric/uint/uint_t.h>
#include <enum/format/format_t.h>
#include <special/consumable/consumable_t.h>

ssize_t        data_complexkey_t              (data_t *data, data_t value, data_t cnext){ // {{{
	complexkey_t          *fdata;
	
	if( (fdata = malloc(sizeof(complexkey_t))) == NULL)
		return -ENOMEM;
	
	fdata->value = value;
	fdata->cnext = cnext;
	
	data->type = TYPE_COMPLEXKEYT;
	data->ptr  = fdata;
	return 0;
} // }}}
void           data_complexkey_t_destroy      (data_t *data){ // {{{
	complexkey_t          *fdata             = (complexkey_t *)data->ptr;
	
	if(fdata){
		data_free(&fdata->value);
		data_free(&fdata->cnext);
		free(fdata);
	}
	data_set_void(data);
} // }}}

static ssize_t data_complexkey_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	switch(fargs->format){
		case FORMAT(native):;
			data_t                     d_value;
			data_t                     d_cnext;
			complexkey_t              *fargs_src        = (complexkey_t *)fargs->src->ptr;
			
			if(data->type != fargs->src->type)
				return -EINVAL;
			
			holder_copy(ret, &d_value, &fargs_src->value);
			if(ret < 0)
				return ret;
			holder_copy(ret, &d_cnext, &fargs_src->cnext);
			if(ret < 0){
				data_free(&d_value);
				return ret;
			}
			
			return data_complexkey_t(data, d_value, d_cnext);
	};
	return -ENOSYS;
} // }}}
static ssize_t data_complexkey_t_free(data_t *data, fastcall_free *fargs){ // {{{
	data_complexkey_t_destroy(data);
	return 0;
} // }}}

static ssize_t data_complexkey_t_lookup(data_t *data, fastcall_lookup *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              key               = 0;
	data_t                 d_key             = DATA_PTR_UINTT(&key);
	complexkey_t          *fdata             = (complexkey_t *)data->ptr;
	
	if(fargs->value == NULL)
		return -EINVAL;
	
	data_get(ret, TYPE_UINTT, key, fargs->key);
	if(ret < 0)
		return ret;
	
	if(key == 0)
		return data_notconsumable_t(fargs->value, fdata->value);
	
	key--;
	
	fastcall_lookup r_lookup = { { 4, ACTION_LOOKUP }, &d_key, fargs->value };
	return data_query(&fdata->cnext, &r_lookup);
} // }}}
static ssize_t data_complexkey_t_consume(data_t *data, fastcall_consume *fargs){ // {{{
	ssize_t                ret;
	
	holder_copy(ret, fargs->dest, data);
	return ret;
} // }}}

data_proto_t complexkey_t_proto = {
	.type                   = TYPE_COMPLEXKEYT,
	.type_str               = "complexkey_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_ENDPOINT,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_complexkey_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_complexkey_t_free,
		
		[ACTION_LOOKUP]       = (f_data_func)&data_complexkey_t_lookup,
		[ACTION_CONSUME]      = (f_data_func)&data_complexkey_t_consume,
	}
};
