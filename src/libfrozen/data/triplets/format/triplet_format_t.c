#include <libfrozen.h>
#include <triplet_format_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>
#include <modifiers/slice/slice_t.h>
#include <modifiers/slider/slider_t.h>
#include <io/io/io_t.h>
#include <storage/raw/raw_t.h>
#include <storage/complexkey/complexkey_t.h>

typedef struct enum_userdata {
	data_t                *data;
	fastcall_enum         *fargs;
} enum_userdata;

static ssize_t data_enum_storage(data_t *storage, data_t *item_template, format_t format, data_t *dest, data_t *key){ // {{{
	ssize_t                ret;
	uintmax_t              offset;
	data_t                 converted;
	data_t                 sl_storage        = DATA_SLIDERT(storage, 0);
	
	do{
		holder_copy(ret, &converted, item_template);
		if(ret < 0)
			break;
		
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &sl_storage, format };
		if( (ret = data_query(&converted, &r_convert)) < -1)
			goto error;
		
		if(ret == -1){
			ret = 0;
			goto error;
		}
		
		offset = data_slider_t_get_offset(&sl_storage);
		data_slider_t_set_offset(&sl_storage, r_convert.transfered, SEEK_CUR);
		
		data_t          d_offset            = DATA_UINTT(offset);
		data_t          d_complex_key_next  = DATA_COMPLEXKEY_NEXTT(d_offset, *key);
		data_t          d_complex_key_end   = DATA_COMPLEXKEY_END_UINTT(offset);
		fastcall_create r_create       = {
			{ 4, ACTION_CREATE },
			data_is_void(key) ? 
				&d_complex_key_end :
				&d_complex_key_next,
			&converted
		};
		ret = data_query(dest, &r_create);
		
		data_free(&converted);
	}while(ret >= 0);
	return ret;
	
error:
	data_free(&converted);
	return ret;
} // }}}

ssize_t data_triplet_format_t(data_t *data, data_t storage, data_t slave){ // {{{
	triplet_format_t      *fdata;
	
	if( (fdata = malloc(sizeof(triplet_format_t))) == NULL )
		return -ENOMEM;
	
	fdata->storage    = storage;
	fdata->slave      = slave;
	
	data->type = TYPE_TRIPLETFORMATT;
	data->ptr  = fdata;
	return 0;
} // }}}
static ssize_t data_triplet_format_t_from_config(data_t *data, config_t *config){ // {{{
	ssize_t                ret;
	data_t                 storage;
	data_t                 slave;
	
	hash_holder_consume(ret, storage, config, HK(data));
	if(ret < 0)
		goto error1;
	
	hash_holder_consume(ret, slave,   config, HK(slave));
	if(ret < 0)
		goto error2;
	
	if( (ret = data_triplet_format_t(data, storage, slave)) < 0)
		goto error3;
	
	return 0;
	
error3:
	data_free(&slave);
error2:
	data_free(&storage);
error1:
	return ret;
} // }}}
static void    data_triplet_format_t_destroy(data_t *data){ // {{{
	triplet_format_t      *fdata             = (triplet_format_t *)data->ptr;
	
	if(fdata){
		data_free(&fdata->storage);
		data_free(&fdata->slave);
		free(fdata);
	}
} // }}}

static ssize_t data_triplet_format_t_iter_iot(data_t *data, enum_userdata *userdata, fastcall_create *fargs){ // {{{
	data_t                 d_void            = DATA_VOID;
	data_t                *key;
	
	if(fargs->value == NULL)
		return 0;
	
	key = fargs->key ? fargs->key : &d_void;
	
	return data_enum_storage(fargs->value, userdata->data, FORMAT(packed), userdata->fargs->dest, key);
} // }}}

static ssize_t data_triplet_format_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			if(dst->ptr != NULL)
				return -EINVAL;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return data_triplet_format_t_from_config(dst, config);
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_triplet_format_t_free(data_t *data, fastcall_free *fargs){ // {{{
	data_triplet_format_t_destroy(data);
	data_set_void(data);
	return 0;
} // }}}
static ssize_t data_triplet_format_t_control(data_t *data, fastcall_control *fargs){ // {{{
	triplet_format_t         *fdata             = (triplet_format_t *)data->ptr;
	
	switch(fargs->function){
		case HK(data):;
			helper_control_data_t r_internal[] = {
				{ HK(data),  &fdata->storage      },
				{ HK(slave), &fdata->slave        },
				{ 0, NULL }
			};
			return helper_control_data(data, fargs, r_internal);
			
		default:
			break;
	}
	return data_default_control(data, fargs);
} // }}}

static ssize_t data_triplet_format_t_crud(data_t *data, fastcall_crud *fargs){ // {{{
	return -ENOSYS;
} // }}}
static ssize_t data_triplet_format_t_lookup(data_t *data, fastcall_crud *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              offset;
	data_t                 converted;
	data_t                 d_key;
	data_t                *d_key_ptr         = &d_key;
	data_t                 storage_item;
	triplet_format_t      *fdata             = (triplet_format_t *)data->ptr;
	
	// fetch value from storage
	data_t          sl_key    = DATA_SLICET(fargs->key, 1, ~0);
	fastcall_lookup r_lookup = {
		{ 4, ACTION_LOOKUP },
		&sl_key,
		&storage_item
	};
	if( (ret = data_query(&fdata->storage, &r_lookup)) < 0)
		return ret;
	
	// get offset for this data
	if(helper_key_current(fargs->key, &d_key) < 0){
		d_key_ptr = fargs->key;
		data_get(ret, TYPE_UINTT, offset, d_key_ptr);
	}else{
		data_get(ret, TYPE_UINTT, offset, d_key_ptr);
		data_free(&d_key);
	}
	
	if(ret < 0)
		return ret;
	
	// prepare copy of template
	holder_copy(ret, &converted, &fdata->slave);
	if(ret < 0)
		goto exit;
	
	data_t                sl_value  = DATA_SLICET(&storage_item, offset, ~0);
	fastcall_convert_from r_convert = {
		{ 5, ACTION_CONVERT_FROM },
		offset == 0 ?
			&storage_item :
			&sl_value,
		FORMAT(packed)
	};
	if( (ret = data_query(&converted, &r_convert)) < 0)
		goto exit;
	
	*fargs->value = converted;
exit:
	data_free(&storage_item);
	return ret;
} // }}}
static ssize_t data_triplet_format_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	triplet_format_t      *fdata             = (triplet_format_t *)data->ptr;
	
	if(data_is_void(&fdata->storage))
		return -EINVAL;
	
	enum_userdata          userdata          = { &fdata->slave, fargs };
	data_t                 iot               = DATA_IOT(&userdata, (f_io_func)&data_triplet_format_t_iter_iot);
	fastcall_enum          r_enum            = { { 3, ACTION_ENUM }, &iot };
	ret = data_query(&fdata->storage, &r_enum);
	
	// last
	fastcall_create r_end = { { 3, ACTION_CREATE }, NULL, NULL };
	data_query(fargs->dest, &r_end);
	
	return ret;
} // }}}

data_proto_t triplet_format_t_proto = {
	.type            = TYPE_TRIPLETFORMATT,
	.type_str        = "triplet_format_t",
	.api_type        = API_HANDLERS,
	.properties      = DATA_PROXY,
	.handlers        = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_triplet_format_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_triplet_format_t_free,
		[ACTION_CONTROL]      = (f_data_func)&data_triplet_format_t_control,
		
		[ACTION_CREATE]       = (f_data_func)&data_triplet_format_t_crud,
		[ACTION_LOOKUP]       = (f_data_func)&data_triplet_format_t_lookup,
		[ACTION_UPDATE]       = (f_data_func)&data_triplet_format_t_crud,
		[ACTION_DELETE]       = (f_data_func)&data_triplet_format_t_crud,
		[ACTION_ENUM]         = (f_data_func)&data_triplet_format_t_enum,
	}
};

static ssize_t data_enum_single_storage(data_t *storage, data_t *item_template, format_t format, data_t *dest, data_t *key){ // {{{
	ssize_t                ret;
	data_t                 converted;
	
	holder_copy(ret, &converted, item_template);
	if(ret < 0)
		return ret;
	
	fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, storage, format };
	if( (ret = data_query(&converted, &r_convert)) < -1)
		goto error;
	
	if(ret == -1){
		ret = 0;
		goto error;
	}
	
	fastcall_create r_create       = { { 4, ACTION_CREATE }, key, &converted };
	ret = data_query(dest, &r_create);
	
error:
	data_free(&converted);
	return ret;
} // }}}
static ssize_t data_triplet_format_single_t_iter_iot(data_t *data, enum_userdata *userdata, fastcall_create *fargs){ // {{{
	data_t                 d_void            = DATA_VOID;
	data_t                *key;
	
	if(fargs->value == NULL)
		return 0;
	
	key = fargs->key ? fargs->key : &d_void;
	
	return data_enum_single_storage(fargs->value, userdata->data, FORMAT(packed), userdata->fargs->dest, key);
} // }}}

static ssize_t data_triplet_format_single_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	switch(fargs->format){
		case FORMAT(hash):;
			ret = data_triplet_format_t_convert_from(dst, fargs);
			
			if(dst->type == TYPE_TRIPLETFORMATT)
				dst->type = TYPE_TRIPLETFORMATSINGLET;
			
			return ret;
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_triplet_format_single_t_lookup(data_t *data, fastcall_crud *fargs){ // {{{
	ssize_t                ret;
	data_t                 converted;
	data_t                 storage_item;
	triplet_format_t      *fdata             = (triplet_format_t *)data->ptr;
	
	// fetch value from storage
	fastcall_lookup r_lookup = {
		{ 4, ACTION_LOOKUP },
		fargs->key,
		&storage_item
	};
	if( (ret = data_query(&fdata->storage, &r_lookup)) < 0)
		return ret;
	
	// prepare copy of template
	holder_copy(ret, &converted, &fdata->slave);
	if(ret < 0)
		goto exit;
	
	fastcall_convert_from r_convert = {
		{ 5, ACTION_CONVERT_FROM },
		&storage_item,
		FORMAT(packed)
	};
	if( (ret = data_query(&converted, &r_convert)) < 0)
		goto exit;
	
	*fargs->value = converted;
exit:
	data_free(&storage_item);
	return ret;
} // }}}
static ssize_t data_triplet_format_single_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	triplet_format_t      *fdata             = (triplet_format_t *)data->ptr;
	
	if(data_is_void(&fdata->storage))
		return -EINVAL;
	
	enum_userdata          userdata          = { &fdata->slave, fargs };
	data_t                 iot               = DATA_IOT(&userdata, (f_io_func)&data_triplet_format_single_t_iter_iot);
	fastcall_enum          r_enum            = { { 3, ACTION_ENUM }, &iot };
	ret = data_query(&fdata->storage, &r_enum);
	
	// last
	fastcall_create r_end = { { 3, ACTION_CREATE }, NULL, NULL };
	data_query(fargs->dest, &r_end);
	
	return ret;
} // }}}

data_proto_t triplet_format_single_t_proto = {
	.type            = TYPE_TRIPLETFORMATSINGLET,
	.type_str        = "triplet_format_single_t",
	.api_type        = API_HANDLERS,
	.properties      = DATA_PROXY,
	.handlers        = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_triplet_format_single_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_triplet_format_t_free,
		[ACTION_CONTROL]      = (f_data_func)&data_triplet_format_t_control,
		
		[ACTION_CREATE]       = (f_data_func)&data_triplet_format_t_crud,
		[ACTION_LOOKUP]       = (f_data_func)&data_triplet_format_single_t_lookup,
		[ACTION_UPDATE]       = (f_data_func)&data_triplet_format_t_crud,
		[ACTION_DELETE]       = (f_data_func)&data_triplet_format_t_crud,
		[ACTION_ENUM]         = (f_data_func)&data_triplet_format_single_t_enum,
	}
};
