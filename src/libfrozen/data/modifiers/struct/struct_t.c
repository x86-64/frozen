#include <libfrozen.h>
#include <struct_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <modifiers/slice/slice_t.h>
#include <modifiers/slider/slider_t.h>

typedef struct struct_iter_ctx {
	request_t             *values;
	data_t                *buffer;
} struct_iter_ctx;

static ssize_t  struct_iter_pack(hash_t *element, void *p_ctx){
	ssize_t                ret;
	format_t               format            = FORMAT(packed);
	hash_t                *element_params;
	data_t                *value;
	struct_iter_ctx       *iter_ctx          = (struct_iter_ctx *)p_ctx;
	
	data_get(ret, TYPE_HASHT, element_params, &element->data);
	if(ret != 0)
		return ITER_BREAK;
	
	hash_data_get(ret, TYPE_FORMATT, format, element_params, HK(format));
	
	if(
		element->key == 0 ||                                                // non anonymous key
		(value = hash_data_find(iter_ctx->values, element->key)) == NULL    // find value for current key
	){
		if( (value = hash_data_find(element_params, HK(default))) == NULL)  // get default value
			return ITER_BREAK;
	}
	// write data to buffer
	fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, iter_ctx->buffer, format };
	if(data_query(value, &r_convert) != 0)
		return ITER_BREAK;
	
	data_slider_t_set_offset(iter_ctx->buffer, r_convert.transfered, SEEK_CUR);
	
	return ITER_CONTINUE;
}

static ssize_t  struct_iter_unpack(hash_t *element, void *p_ctx){
	ssize_t                ret;
	format_t               format            = FORMAT(packed);
	hash_t                *element_params;
	data_t                *value;
	data_t                 need_data;
	uintmax_t              need_data_free    = 0;
	struct_iter_ctx       *iter_ctx          = (struct_iter_ctx *)p_ctx;
	
	data_get(ret, TYPE_HASHT, element_params, &element->data);
	if(ret != 0)
		return ITER_BREAK;
	
	hash_data_get(ret, TYPE_FORMATT, format, element_params, HK(format));
	
	// prepare data
	if( (value = hash_data_find(iter_ctx->values, element->key)) == NULL){      // find destination data for current key
		if( (value = hash_data_find(element_params, HK(default))) == NULL)  // if not - copy default value
			return ITER_BREAK;
		
		holder_copy(ret, &need_data, value);
		if(ret < 0)
			return ITER_BREAK;
		
		value          = &need_data;
		need_data_free = 1;
	}
	
	// read data from buffer
	fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_FROM }, iter_ctx->buffer, format };
	if(data_query(value, &r_convert) < 0)
		return ITER_BREAK;
	
	data_slider_t_set_offset(iter_ctx->buffer, r_convert.transfered, SEEK_CUR);
	
	if(need_data_free == 1){
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(value, &r_free);
	}
	
	return ITER_CONTINUE;
}

/** Pack hash with values, using structure into buffer */
uintmax_t  struct_pack      (hash_t *structure, request_t *values, data_t *buffer){
	data_t                 sl_buffer         = DATA_SLIDERT(buffer, 0);
	struct_iter_ctx        iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = &sl_buffer;
	
	if(hash_iter(structure, &struct_iter_pack, &iter_ctx, 0) == ITER_BREAK)
		return 0;
	
	return data_slider_t_get_offset(&sl_buffer);
}

/** Unpack buffer to values using structure */
uintmax_t  struct_unpack    (hash_t *structure, request_t *values, data_t *buffer){
	data_t                 sl_buffer         = DATA_SLIDERT(buffer, 0);
	struct_iter_ctx        iter_ctx;
	
	iter_ctx.values      = values;
	iter_ctx.buffer      = &sl_buffer;
	
	if(hash_iter(structure, &struct_iter_unpack, &iter_ctx, 0) == ITER_BREAK)
		return 0;
	
	return data_slider_t_get_offset(&sl_buffer);
}

static ssize_t   data_struct_t_convert_to(data_t *data, fastcall_convert_to *fargs){ // {{{
	struct_t              *fdata             = (struct_t *)(data->ptr);
	
	if(fargs->dest == NULL || fdata == NULL || fdata->values == NULL || fdata->structure == NULL)
		return -EINVAL;
	
	if(fargs->format != FORMAT(native))
		return -EINVAL;
	
	data_t                 sl_buffer         = DATA_SLIDERT(fargs->dest, 0);
	struct_iter_ctx        iter_ctx;
	
	iter_ctx.values      = fdata->values;
	iter_ctx.buffer      = &sl_buffer;
	
	if(hash_iter(fdata->structure, &struct_iter_pack, &iter_ctx, 0) == ITER_BREAK)
		return -EFAULT;
	
	if(fargs->header.nargs >= 5)
		fargs->transfered = data_slider_t_get_offset(&sl_buffer);
	
	return 0; 
} // }}}

data_proto_t struct_t_proto = {
	.type                   = TYPE_STRUCTT,
	.type_str               = "struct_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO] = (f_data_func)&data_struct_t_convert_to,
	}
};

