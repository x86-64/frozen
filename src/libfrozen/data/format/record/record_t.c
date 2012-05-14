#include <libfrozen.h>
#include <record_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>
#include <modifiers/slice/slice_t.h>
#include <modifiers/slider/slider_t.h>
#include <io/io/io_t.h>
#include <storage/raw/raw_t.h>

typedef struct enum_userdata {
	data_t                *data;
	fastcall_enum         *fargs;
} enum_userdata;

ssize_t data_strstr(data_t *haystack, data_t *separator, uintmax_t *offset, uintmax_t *sep_len){ // {{{
	ssize_t                ret;
	void                  *separator_ptr;
	uintmax_t              separator_len;
	char                  *match;
	char                   buffer_stack[DEF_BUFFER_SIZE];
	char                  *buffer;
	uintmax_t              buffer_left;
	uintmax_t              original_offset   = 0;
	uintmax_t              original_eoffset;
	
	data_t                 sl_haystack           = DATA_AUTO_SLIDERT(haystack, 0);
	
	fastcall_view  r_view = { { 6, ACTION_VIEW }, FORMAT(native) };
	if( (ret = data_query(separator, &r_view)) < 0) 
		return ret;
	
	separator_ptr = r_view.ptr;
	separator_len = r_view.length;
	if(separator_len > DEF_BUFFER_SIZE){
		data_free(&r_view.freeit);
		return -EINVAL;
	}
	
	while(1){
		fastcall_read r_read = { { 5, ACTION_READ }, 0, buffer_stack, sizeof(buffer_stack) };
		if( (ret = data_query(&sl_haystack, &r_read)) < 0)
			break;
		
		buffer            = buffer_stack;
		buffer_left       = r_read.buffer_size;
		original_eoffset  = original_offset + r_read.buffer_size;
		
		while(1){
			if( (match = memchr(buffer, (int)((char *)separator_ptr)[0], buffer_left)) == NULL)
				break;
			
			buffer_left -= (match - buffer);
			
			if(buffer_left < separator_len){
				// fill buffer
				memcpy(buffer_stack, match, buffer_left);
				
				fastcall_read r_read = { { 5, ACTION_READ }, 0, buffer_stack + buffer_left, sizeof(buffer_stack) - buffer_left };
				if( (ret = data_query(&sl_haystack, &r_read)) < 0)
					goto exit;
				
				buffer            = buffer_stack;
				buffer_left      += r_read.buffer_size;
				original_offset  += (match - buffer_stack);
				original_eoffset  = original_offset + r_read.buffer_size;
				continue;
			}
			
			if(memcmp(match, separator_ptr, separator_len) == 0){
				// found match
				*offset  = original_offset + (match - buffer_stack);
				*sep_len = separator_len;
				ret      = 0;
				goto exit;
			}
			
			buffer         = match + 1;
			buffer_left   -= 1;
		}
		
		original_offset = original_eoffset;
	}
exit:
	data_free(&r_view.freeit);
	return ret;
} // }}}
ssize_t data_enum_storage(data_t *storage, data_t *item_template, format_t format, data_t *dest){ // {{{
	ssize_t                ret;
	uintmax_t              offset;
	data_t                 sl_storage        = DATA_SLIDERT(storage, 0);
	
	do{
		
		fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &sl_storage, format };
		if( (ret = data_query(item_template, &r_convert)) < -1)
			break;
		
		if(ret == -1)
			return 0;
		
		offset = data_slider_t_get_offset(&sl_storage);
		data_t          key      = DATA_HEAP_UINTT(offset);
		
		data_slider_t_set_offset(&sl_storage, r_convert.transfered, SEEK_CUR);
		
		fastcall_create r_create = { { 4, ACTION_CREATE }, &key, item_template };
		ret = data_query(dest, &r_create);
		
		data_free(&key);
	}while(ret >= 0);
	return ret;
} // }}}

ssize_t data_record_t_iter_iot(data_t *data, enum_userdata *userdata, fastcall_create *fargs){ // {{{
	if(fargs->value == NULL)
		return 0;
	
	return data_enum_storage(fargs->value, userdata->data, FORMAT(packed), userdata->fargs->dest);
} // }}}

record_t *        record_t_alloc(data_t *data, data_t *separator, uintmax_t consume){ // {{{
	record_t                 *fdata;
	
	if( (fdata = malloc(sizeof(record_t))) == NULL )
		return NULL;
	
	data_set_void(&fdata->storage);
	data_set_void(&fdata->separator);
	data_raw_t_empty(&fdata->item);
	
	if(data){
		fdata->storage       = *data;
		if(consume)
			data_set_void(data);
	}
	if(separator){
		fdata->separator  = *separator;
		if(consume)
			data_set_void(separator);
	}
	return fdata;
} // }}}
void              record_t_destroy(record_t *record){ // {{{
	data_free(&record->storage);
	data_free(&record->separator);
	data_free(&record->item);
	free(record);
} // }}}

static ssize_t data_record_t_handler (data_t *data, fastcall_header *fargs){ // {{{
	record_t               *fdata             = (record_t *)data->ptr;
	
	return data_query(&fdata->item, fargs);
} // }}}
static ssize_t data_record_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              transfered        = 0;
	record_t              *fdata             = (record_t *)src->ptr;
	data_t                 sl_dest           = DATA_SLIDERT(fargs->dest, 0);
	
	switch(fargs->format){
		case FORMAT(packed):;
			fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, &sl_dest, FORMAT(native) };
			
			// <data>
			ret         = data_query(&fdata->item,      &r_convert);
			transfered += r_convert.transfered;
			
			data_slider_t_set_offset(&sl_dest, r_convert.transfered, SEEK_CUR);
			
			if(ret < 0)
				break;
			
			// <separator>
			ret         = data_query(&fdata->separator, &r_convert);
			transfered += r_convert.transfered;
			break;
			
		default:
			return data_query(&fdata->item, fargs);
	};
	if(fargs->header.nargs >= 5)
		fargs->transfered = transfered;
	
	return ret;
} // }}}
static ssize_t data_record_t_length(data_t *data, fastcall_length *fargs){ // {{{
	ssize_t                    ret;
	record_t                  *fdata             = (record_t *)data->ptr;
	
	switch(fargs->format){
		case FORMAT(packed):;
			fargs->length = 0;
			
			fastcall_length r_length = { { 5, ACTION_LENGTH }, 0, FORMAT(native) };
			ret            = data_query(&fdata->item,      &r_length);
			fargs->length += r_length.length;
			
			if(ret < 0)
				break;
			
			ret            = data_query(&fdata->separator, &r_length);
			fargs->length += r_length.length;
			break;
			
		default:
			ret = data_query(&fdata->item, fargs);
			break;
	}
	return ret;
} // }}}

static ssize_t data_record_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	data_t                *storage;
	data_t                *separator;
	record_t              *fdata             = (record_t *)dst->ptr;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			if(fdata != NULL)
				return -EINVAL;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			storage    = hash_data_find(config, HK(data));
			separator  = hash_data_find(config, HK(separator));
			
			if( (fdata = dst->ptr = record_t_alloc(storage, separator, 1)) == NULL)
				return -ENOMEM;
			
			return 0;
		
		case FORMAT(native):;
			data_t                 cp_storage;
			data_t                 cp_separator;
			record_t              *src_fdata          = (record_t *)fargs->src->ptr;
			
			if(dst->type != fargs->src->type || src_fdata == NULL)
				return -EINVAL;
			
			holder_copy(ret, &cp_storage,   &src_fdata->storage);
			holder_copy(ret, &cp_separator, &src_fdata->separator);
			
			if( (fdata = dst->ptr = record_t_alloc(&cp_storage, &cp_separator, 1)) == NULL)
				return -ENOMEM;
			
			return 0;
		
		case FORMAT(packed):;
			uintmax_t              offset;
			uintmax_t              sep_len;
			
			if( (ret = data_strstr(fargs->src, &fdata->separator, &offset, &sep_len)) < 0)
				return ret;
			
			data_t                 sl_src           = DATA_SLICET(fargs->src, 0, offset);
			fastcall_convert_from  r_convert        = { { 5, ACTION_CONVERT_FROM }, &sl_src, FORMAT(native) };
			if( (ret = data_query(&fdata->item, &r_convert)) < 0)
				return ret;
			
			fargs->transfered = offset + sep_len;
			return 0;
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_record_t_free(data_t *data, fastcall_free *fargs){ // {{{
	record_t                  *fdata             = (record_t *)data->ptr;
	
	record_t_destroy(fdata);
	data_set_void(data);
	return 0;
} // }}}

static ssize_t data_record_t_crud(data_t *data, fastcall_crud *fargs){ // {{{
	//record_t                  *fdata             = (record_t *)data->ptr;
	
	return -ENOSYS;
} // }}}
static ssize_t data_record_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	record_t              *fdata             = (record_t *)data->ptr;
	
	if(fdata->storage.type == TYPE_VOIDT) // if(data_is_void(&fdata->storage))
		return -EINVAL;
	
	enum_userdata          userdata          = { data, fargs };
	data_t                 iot               = DATA_IOT(&userdata, (f_io_func)&data_record_t_iter_iot);
	fastcall_enum          r_enum            = { { 3, ACTION_ENUM }, &iot };
	ret = data_query(&fdata->storage, &r_enum);
	
	// last
	fastcall_create r_end = { { 3, ACTION_CREATE} , NULL, NULL };
	data_query(fargs->dest, &r_end);
	
	return ret;
} // }}}

data_proto_t record_t_proto = {
	.type            = TYPE_RECORDT,
	.type_str        = "record_t",
	.api_type        = API_HANDLERS,
	.properties      = DATA_PROXY,
	.handlers        = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_record_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_record_t_free,
		
		[ACTION_CONVERT_TO]   = (f_data_func)&data_record_t_convert_to,
		[ACTION_LENGTH]       = (f_data_func)&data_record_t_length,
		
		[ACTION_RESIZE]       = (f_data_func)&data_record_t_handler,
		[ACTION_READ]         = (f_data_func)&data_record_t_handler,
		[ACTION_WRITE]        = (f_data_func)&data_record_t_handler,
		[ACTION_LENGTH]       = (f_data_func)&data_record_t_handler,
		
		[ACTION_CREATE]       = (f_data_func)&data_record_t_crud,
		[ACTION_LOOKUP]       = (f_data_func)&data_record_t_crud,
		[ACTION_UPDATE]       = (f_data_func)&data_record_t_crud,
		[ACTION_DELETE]       = (f_data_func)&data_record_t_crud,
		[ACTION_ENUM]         = (f_data_func)&data_record_t_enum,
	}
};

