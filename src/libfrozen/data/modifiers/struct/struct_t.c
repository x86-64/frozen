#include <libfrozen.h>
#include <dataproto.h>
#include <struct_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <modifiers/slice/slice_t.h>
#include <modifiers/slider/slider_t.h>

static ssize_t   data_struct_t_read  (data_t *data, fastcall_read *fargs){
	return -1;
}
static ssize_t   data_struct_t_write (data_t *data, fastcall_write *fargs){
	return -1;
}

typedef struct struct_iter_ctx {
	request_t             *values;
	data_t                *buffer;
} struct_iter_ctx;

static ssize_t  struct_iter_pack(hash_t *element, void *p_ctx){
	ssize_t                ret;
	format_t               format            = FORMAT(binary);
	hash_t                *element_params;
	data_t                *value;
	struct_iter_ctx       *iter_ctx          = (struct_iter_ctx *)p_ctx;
	
	data_get(ret, TYPE_HASHT, element_params, &element->data);
	if(ret != 0)
		return ITER_BREAK;
	
	hash_data_copy(ret, TYPE_FORMATT, format, element_params, HK(format));
	
	if( (value = hash_data_find(iter_ctx->values, element->key)) == NULL){      // find value for current key
		if( (value = hash_data_find(element_params, HK(default))) == NULL)  // get default value
			return ITER_BREAK;
	}
	
	data_slider_t_freeze(iter_ctx->buffer);
	
	// write data to buffer
	fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_TO }, iter_ctx->buffer, format };
	if(data_query(value, &r_convert) != 0)
		return ITER_BREAK;
	
	data_slider_t_unfreeze(iter_ctx->buffer);
	
	return ITER_CONTINUE;
}

static ssize_t  struct_iter_unpack(hash_t *element, void *p_ctx){
	ssize_t                ret;
	format_t               format            = FORMAT(binary);
	hash_t                *element_params;
	data_t                *value;
	data_t                 need_data;
	uintmax_t              need_data_free    = 0;
	struct_iter_ctx       *iter_ctx          = (struct_iter_ctx *)p_ctx;
	
	data_get(ret, TYPE_HASHT, element_params, &element->data);
	if(ret != 0)
		return ITER_BREAK;
	
	hash_data_copy(ret, TYPE_FORMATT, format, element_params, HK(format));
	
	// prepare data
	if( (value = hash_data_find(iter_ctx->values, element->key)) == NULL){      // find destination data for current key
		if( (value = hash_data_find(element_params, HK(default))) == NULL)  // if not - copy default value
			return ITER_BREAK;
		
		fastcall_copy r_copy = { { 3, ACTION_COPY }, &need_data };
		if(data_query(value, &r_copy) != 0)
			return ITER_BREAK;
		
		value          = &need_data;
		need_data_free = 1;
	}
	
	data_slider_t_freeze(iter_ctx->buffer);

	// read data from buffer
	fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_FROM }, iter_ctx->buffer, format };
	if(data_query(value, &r_convert) < 0)
		return ITER_BREAK;
	
	data_slider_t_unfreeze(iter_ctx->buffer);
	
	if(need_data_free == 1){
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(value, &r_free);
	}
	
	return ITER_CONTINUE;
}

/** Pack hash with values, using structure into buffer */
uintmax_t  struct_pack      (struct_t *structure, request_t *values, data_t *buffer){
	data_t                 sl_buffer         = DATA_SLIDERT(buffer, 0);
	struct_iter_ctx        iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = &sl_buffer;
	
	if(hash_iter(structure, &struct_iter_pack, &iter_ctx, 0) == ITER_BREAK)
		return 0;
	
	return data_slider_t_get_offset(&sl_buffer);
}

/** Unpack buffer to values using structure */
uintmax_t  struct_unpack    (struct_t *structure, request_t *values, data_t *buffer){
	data_t                 sl_buffer         = DATA_SLIDERT(buffer, 0);
	struct_iter_ctx        iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = &sl_buffer;
	
	if(hash_iter(structure, &struct_iter_unpack, &iter_ctx, 0) == ITER_BREAK)
		return 0;
	
	return data_slider_t_get_offset(&sl_buffer);
}
	
data_proto_t struct_t_proto = {
	.type                   = TYPE_STRUCTT,
	.type_str               = "struct_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_READ]   = (f_data_func)&data_struct_t_read,
		[ACTION_WRITE]  = (f_data_func)&data_struct_t_write
	}
};

