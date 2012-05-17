#include <libfrozen.h>
#include <triplet_key_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>
#include <io/io/io_t.h>

typedef struct enum_userdata {
	data_t                *data;
	fastcall_enum         *fargs;
} enum_userdata;

ssize_t data_triplet_key_t(data_t *data, data_t storage, data_t slave){ // {{{
	triplet_key_t         *fdata;
	
	if( (fdata = malloc(sizeof(triplet_key_t))) == NULL){
		data_set_void(data);
		return -ENOMEM;
	}
	
	fdata->storage = storage;
	fdata->slave   = slave;
	
	data->type = TYPE_TRIPLETKEYT;
	data->ptr  = fdata;
	return 0;
} // }}}
ssize_t data_triplet_key_t_from_config(data_t *data, config_t *config){ // {{{
	ssize_t                ret;
	data_t                 storage           = DATA_VOID;
	data_t                 slave             = DATA_VOID;
	
	hash_holder_consume(ret, storage, config, HK(data));
	hash_holder_consume(ret, slave,   config, HK(slave));
	
	return data_triplet_key_t(data, storage, slave);
} // }}}
void    data_triplet_key_t_destroy(data_t *data){ // {{{
	triplet_key_t         *fdata             = (triplet_key_t *)data->ptr;
	
	if(fdata){
		data_free(&fdata->storage);
		data_free(&fdata->slave);
		free(fdata);
	}
	data_set_void(data);
} // }}}

static ssize_t data_triplet_key_t_iter_iot(data_t *iot_data, enum_userdata *userdata, fastcall_create *fargs){ // {{{
	ssize_t                ret;
	data_t                 d_value           = DATA_VOID;
	triplet_key_t         *fdata             = (triplet_key_t *)userdata->data->ptr;
	
	if(fargs->value == NULL)
		return 0;
	
	fastcall_lookup r_lookup = { { 4, ACTION_LOOKUP }, fargs->value, &d_value };
	if( (ret = data_query(&fdata->storage, &r_lookup)) < 0)
		return ret;
	
	fastcall_create r_create = { { 4, ACTION_CREATE }, fargs->key, &d_value };
	ret = data_query(userdata->fargs->dest, &r_create);
	
	data_free(&d_value);
	return ret;
} // }}}

static ssize_t data_triplet_key_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	triplet_key_t         *fdata             = (triplet_key_t *)dst->ptr;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			if(fdata != NULL)
				return -EINVAL;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return data_triplet_key_t_from_config(dst, config);
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_triplet_key_t_free(data_t *data, fastcall_free *fargs){ // {{{
	data_triplet_key_t_destroy(data);
	return 0;
} // }}}

static ssize_t data_triplet_key_t_handler(data_t *data, fastcall_header *fargs){ // {{{
	triplet_key_t         *fdata             = (triplet_key_t *)data->ptr;
	
	return data_query(&fdata->storage, fargs);
} // }}}
static ssize_t data_triplet_key_t_c(data_t *data, fastcall_crud *fargs){ // {{{
	ssize_t                ret;
	data_t                 d_key             = DATA_VOID;
	triplet_key_t         *fdata             = (triplet_key_t *)data->ptr;
	
	// save item to storage using empty key.
	fastcall_create r_create_item = { { 4, ACTION_CREATE }, &d_key, fargs->value };
	if( (ret = data_query(&fdata->storage, &r_create_item)) < 0)
		return ret;
	
	// save key to slave data
	fastcall_create r_create_key = { { 4, ACTION_CREATE }, fargs->key, &d_key };
	ret = data_query(&fdata->slave, &r_create_key);
	
	data_free(&d_key);
	return ret;
} // }}}
static ssize_t data_triplet_key_t_ru(data_t *data, fastcall_crud *fargs){ // {{{
	ssize_t                ret;
	data_t                 d_key             = DATA_VOID;
	triplet_key_t         *fdata             = (triplet_key_t *)data->ptr;
	
	// lookup storage key from slave data
	fastcall_lookup r_lookup_key = { { 4, ACTION_LOOKUP }, fargs->key, &d_key };
	if( (ret = data_query(&fdata->slave, &r_lookup_key)) < 0)
		return ret;
	
	// make request to storage with given key
	fastcall_crud   r_crud_item  = { { 4, fargs->header.action }, &d_key, fargs->value };
	ret = data_query(&fdata->storage, &r_crud_item);
	
	data_free(&d_key);
	return ret;
} // }}}
static ssize_t data_triplet_key_t_d(data_t *data, fastcall_crud *fargs){ // {{{
	ssize_t                ret;
	data_t                 d_key             = DATA_VOID;
	triplet_key_t         *fdata             = (triplet_key_t *)data->ptr;
	
	// lookup storage key from slave data
	fastcall_lookup r_lookup_key = { { 4, ACTION_LOOKUP }, fargs->key, &d_key };
	if( (ret = data_query(&fdata->slave, &r_lookup_key)) < 0)
		return ret;
	
	// delete item from storage using given key
	fastcall_delete r_delete_item = { { 4, ACTION_DELETE }, &d_key, fargs->value };
	if( (ret = data_query(&fdata->storage, &r_delete_item)) < 0)
		goto exit;
	
	// delete key from slave data
	fastcall_delete r_delete_key = { { 4, ACTION_DELETE }, fargs->key, NULL };
	ret = data_query(&fdata->slave, &r_delete_key);
	
exit:
	data_free(&d_key);
	return ret;
} // }}}
static ssize_t data_triplet_key_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	triplet_key_t         *fdata             = (triplet_key_t *)data->ptr;
	
	// enumerate slave data, because it have all valid keys to storage data
	enum_userdata          userdata          = { data, fargs };
	data_t                 iot               = DATA_IOT(&userdata, (f_io_func)&data_triplet_key_t_iter_iot);
	fastcall_enum          r_enum            = { { 3, ACTION_ENUM }, &iot };
	ret = data_query(&fdata->slave, &r_enum);
	
	// last
	fastcall_create r_end = { { 3, ACTION_CREATE }, NULL, NULL };
	data_query(fargs->dest, &r_end);
	return ret;
} // }}}
static ssize_t data_triplet_key_t_control(data_t *data, fastcall_control *fargs){ // {{{
	triplet_key_t         *fdata             = (triplet_key_t *)data->ptr;
	
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

data_proto_t triplet_key_t_proto = {
	.type            = TYPE_TRIPLETKEYT,
	.type_str        = "triplet_key_t",
	.api_type        = API_HANDLERS,
	.properties      = DATA_PROXY,
	.handler_default = (f_data_func)&data_triplet_key_t_handler,
	.handlers        = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_triplet_key_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_triplet_key_t_free,
		[ACTION_CONTROL]      = (f_data_func)&data_triplet_key_t_control,
		
		[ACTION_CREATE]       = (f_data_func)&data_triplet_key_t_c,
		[ACTION_LOOKUP]       = (f_data_func)&data_triplet_key_t_ru,
		[ACTION_UPDATE]       = (f_data_func)&data_triplet_key_t_ru,
		[ACTION_DELETE]       = (f_data_func)&data_triplet_key_t_d,
		[ACTION_ENUM]         = (f_data_func)&data_triplet_key_t_enum,
	}
};

