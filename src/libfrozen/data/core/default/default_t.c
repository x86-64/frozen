#include <libfrozen.h>

#include <core/void/void_t.h>
#include <core/string/string_t.h>
#include <enum/format/format_t.h>
#include <enum/hashkey/hashkey_t.h>
#include <special/immortal/immortal_t.h>
#include <storage/raw/raw_t.h>

ssize_t default_transfer(data_t *src, data_t *dest, uintmax_t read_offset, uintmax_t write_offset, uintmax_t size, uintmax_t *ptransfered){ // {{{
	ssize_t         ret;
	uintmax_t       roffset                  = read_offset;
	uintmax_t       woffset                  = write_offset;
	uintmax_t       bytes_written            = 0;
	uintmax_t       bytes_read;
	char            buffer[DEF_BUFFER_SIZE];
	
	// first read
	fastcall_read r_read = { { 5, ACTION_READ }, roffset, &buffer, MIN(size, sizeof(buffer)) };
	if( (ret = data_query(src, &r_read)) < 0) // EOF return too
		return ret;
	
	bytes_read   = r_read.buffer_size;
	roffset     += r_read.buffer_size;
	size        -= r_read.buffer_size;
	
	goto start;
	do {
		fastcall_read r_read = { { 5, ACTION_READ }, roffset, &buffer, MIN(size, sizeof(buffer)) };
		if( (ret = data_query(src, &r_read)) < -1)
			return ret;
		
		if(ret == -1) // EOF from read side
			break;
		
		bytes_read  = r_read.buffer_size;
		roffset    += r_read.buffer_size;
		size       -= r_read.buffer_size;
		
	start:;
		fastcall_write r_write = { { 5, ACTION_WRITE }, woffset, &buffer, bytes_read };
		if( (ret = data_query(dest, &r_write)) < -1)
			return ret;
		
		if(ret == -1) // EOF from write side
			break;
		
		bytes_written += r_write.buffer_size;
		woffset       += r_write.buffer_size;
	}while(size > 0);
	
	if(ptransfered)
		*ptransfered = bytes_written;
	return 0;
} // }}}

ssize_t       data_default_read          (data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret;
	
	fastcall_view  r_view = { { 6, ACTION_VIEW }, FORMAT(native) };
	if( (ret = data_query(data, &r_view)) < 0)
		return ret;
	
	if(r_view.length == 0){
		fargs->buffer_size = 0;
		data_free(&r_view.freeit);
		return -1; // EOF
	}
	
	if(fargs->buffer == NULL || fargs->offset > r_view.length){
		data_free(&r_view.freeit);
		return -EINVAL; // invalid range
	}
	
	fargs->buffer_size = MIN(fargs->buffer_size, r_view.length - fargs->offset);
	
	if(fargs->buffer_size == 0){
		data_free(&r_view.freeit);
		return -1; // EOF
	}
	
	memcpy(fargs->buffer, r_view.ptr + fargs->offset, fargs->buffer_size);
	data_free(&r_view.freeit);
	return 0;
} // }}}
ssize_t       data_default_write         (data_t *data, fastcall_write *fargs){ // {{{
	ssize_t                ret;
	
	fastcall_view  r_view = { { 6, ACTION_VIEW }, FORMAT(native) };
	if( (ret = data_query(data, &r_view)) < 0)
		return ret;
	
	if(r_view.length == 0){
		fargs->buffer_size = 0;
		data_free(&r_view.freeit);
		return -1; // EOF
	}
	
	if(fargs->buffer == NULL || fargs->offset > r_view.length){
		data_free(&r_view.freeit);
		return -EINVAL; // invalid range
	}
	
	fargs->buffer_size = MIN(fargs->buffer_size, r_view.length - fargs->offset);
	
	if(fargs->buffer_size == 0){
		data_free(&r_view.freeit);
		return -1; // EOF
	}
	
	memcpy(r_view.ptr + fargs->offset, fargs->buffer, fargs->buffer_size);
	data_free(&r_view.freeit);
	return 0;
} // }}}

ssize_t       data_default_compare       (data_t *data1, fastcall_compare *fargs){ // {{{
	ssize_t                ret;
	char                   buffer1[DEF_BUFFER_SIZE], buffer2[DEF_BUFFER_SIZE];
	uintmax_t              buffer1_size, buffer2_size, cmp_size;
	uintmax_t              offset1, offset2, goffset1, goffset2;
	
	if(fargs->data2 == NULL)
		return -EINVAL;
	
	goffset1     = goffset2     = 0;
	buffer1_size = buffer2_size = 0;
	do {
		if(buffer1_size == 0){
			fastcall_read r_read = { { 5, ACTION_READ }, goffset1, &buffer1, sizeof(buffer1) };
			if( (ret = data_query(data1, &r_read)) < -1)
				return ret;
			
			if(ret == -1 && buffer2_size != 0){
				fargs->result = 1;
				break;
			}
			
			buffer1_size  = r_read.buffer_size;
			goffset1     += r_read.buffer_size;
			offset1       = 0;
		}
		if(buffer2_size == 0){
			fastcall_read r_read = { { 5, ACTION_READ }, goffset2, &buffer2, sizeof(buffer2) };
			if( (ret = data_query(fargs->data2, &r_read)) < -1)
				return ret;
			
			if(ret == -1 && buffer1_size != 0){
				fargs->result = 2;
				break;
			}
			if(ret == -1){
				fargs->result = 0;
				break;
			}
			
			buffer2_size  = r_read.buffer_size;
			goffset2     += r_read.buffer_size;
			offset2       = 0;
		}
		
		cmp_size = MIN(buffer1_size, buffer2_size);
		
		if( (ret = memcmp(buffer1 + offset1, buffer2 + offset2, cmp_size)) != 0){
			fargs->result = (ret < 0) ? 1 : 2;
			break;
		}
		
		offset1      += cmp_size;
		offset2      += cmp_size;
		buffer1_size -= cmp_size;
		buffer2_size -= cmp_size;
	}while(1);
	return 0;

} // }}}

ssize_t       data_default_view          (data_t *data, fastcall_view *fargs){ // {{{
	ssize_t                ret;
	
	switch(fargs->format){
		case FORMAT(native):;
			if(data->ptr == NULL)
				return -EINVAL;
			
			fastcall_length r_len = { { 3, ACTION_LENGTH }, 0, FORMAT(native) };
			data_query(data, &r_len);
			
			fargs->ptr    = data->ptr;
			fargs->length = r_len.length;
			data_set_void(&fargs->freeit);
			return 0;
			
		default:;
			data_t           d_view         = DATA_RAWT_EMPTY();
			
			fastcall_convert_to r_convert = { { 5, ACTION_CONVERT_TO }, &d_view, fargs->format };
			if( (ret = data_query(data, &r_convert)) < 0)
				return ret;
			
			fastcall_view       r_view    = { { 6, ACTION_VIEW }, FORMAT(native) };
			if( (ret = data_query(&d_view, &r_view)) < 0){
				data_free(&d_view);
				return ret;
			}
			
			fargs->ptr    = r_view.ptr;
			fargs->length = r_view.length;
			fargs->freeit = d_view;
			return 0;
	}
	return -ENOSYS;
} // }}}
ssize_t       data_default_is_null       (data_t *data, fastcall_is_null *fargs){ // {{{
	fargs->is_null = (data->ptr == NULL) ? 1 : 0;
	return 0;
} // }}}

ssize_t       data_default_convert_to    (data_t *src, fastcall_convert_to *fargs){ // {{{
	uintmax_t             *transfered;
	
	if(fargs->dest == NULL)
		return -EINVAL;
	
	if(fargs->format != FORMAT(native))
		return -ENOSYS;
	
	transfered = (fargs->header.nargs >= 5) ? &fargs->transfered : NULL;
	
	return default_transfer(src, fargs->dest, 0, 0, ~0, transfered);
} // }}}
ssize_t       data_default_convert_from  (data_t *dest, fastcall_convert_from *fargs){ // {{{
	switch(fargs->format){
		case FORMAT(native):;
			fastcall_convert_to r_convert = { { 3, ACTION_CONVERT_TO }, dest, FORMAT(native) };
			return data_query(fargs->src, &r_convert);
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
ssize_t       data_default_free          (data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr != NULL)
		free(data->ptr);
	return 0;
} // }}}
ssize_t       data_default_consume       (data_t *data, fastcall_consume *fargs){ // {{{
	//ssize_t                ret;
	//holder_copy(ret, fargs->dest, data)
	//return ret;
	*fargs->dest = *data;
	data->ptr = NULL;
	return 0;
} // }}}
ssize_t       data_default_control       (data_t *data, fastcall_control *fargs){ // {{{
	switch(fargs->function){
		case HK(datatype):;
			data_t                d_type    = DATA_PTR_DATATYPET(&data->type);
			fastcall_convert_from r_convert = { { 5, ACTION_CONVERT_FROM }, &d_type, FORMAT(native) };
			return data_query(fargs->value, &r_convert);
		
		case HK(data):;
			data_t                d_key     = DATA_VOID;
			
			if(fargs->key && helper_key_current(fargs->key, &d_key) == 0){ // expect only empty key
				data_free(&d_key);
				return -EINVAL;
			}
			
			fargs->value = data;
			return 0;
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}

ssize_t       data_default_enum          (data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	data_t                 immortal          = DATA_IMMORTALT(data);
	fastcall_create        r_create          = { { 4, ACTION_CREATE }, NULL, &immortal };
	fastcall_create        r_end             = { { 4, ACTION_CREATE }, NULL, NULL      };
	
	ret = data_query(fargs->dest, &r_create);
	
	data_query(fargs->dest, &r_end);
	
	return ret;
} // }}}

data_proto_t default_t_proto = {
	.type            = TYPE_DEFAULTT,
	.type_str        = "",
	.api_type        = API_HANDLERS,
	.handlers        = {
		[ACTION_READ]        = (f_data_func)&data_default_read,
		[ACTION_WRITE]       = (f_data_func)&data_default_write,
		
		[ACTION_COMPARE]     = (f_data_func)&data_default_compare,
		
		[ACTION_VIEW]        = (f_data_func)&data_default_view,
		[ACTION_IS_NULL]     = (f_data_func)&data_default_is_null,
		
		[ACTION_CONVERT_TO]  = (f_data_func)&data_default_convert_to,
		[ACTION_CONVERT_FROM]= (f_data_func)&data_default_convert_from,
		[ACTION_FREE]        = (f_data_func)&data_default_free,
		[ACTION_CONTROL]     = (f_data_func)&data_default_control,
		[ACTION_CONSUME]     = (f_data_func)&data_default_consume,
		
		[ACTION_ENUM]        = (f_data_func)&data_default_enum,
	}
};

