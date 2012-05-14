#include <libfrozen.h>
#include <slider_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>

slider_t *        slider_t_alloc(data_t *data, uintmax_t offset, uintmax_t auto_slide){ // {{{
	slider_t                 *fdata;
	
	if( (fdata = malloc(sizeof(slider_t))) == NULL )
		return NULL;
	
	fdata->data       = data;
	fdata->off        = offset;
	fdata->auto_slide = auto_slide;
	
	data_set_void(&fdata->freeit);
	return fdata;
} // }}}
void              slider_t_destroy(slider_t *slider){ // {{{
	data_free(&slider->freeit);
	free(slider);
} // }}}

uintmax_t data_slider_t_get_offset(data_t *data){ // {{{
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	return fdata->off;
} // }}}
ssize_t   data_slider_t_set_offset(data_t *data, intmax_t offset, uintmax_t dir){ // {{{
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	switch(dir){
		case SEEK_SET: fdata->off  = offset; break;
		case SEEK_CUR: fdata->off += offset; break;
		default:
			return -EINVAL;
	}
	return 0;
} // }}}

static ssize_t data_slider_t_handler (data_t *data, fastcall_header *fargs){ // {{{
	size_t                     ret;
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	switch(fargs->action){
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io         *ioargs     = (fastcall_io *)fargs;
			
			ioargs->offset      += fdata->off;
			
			if( (ret = data_query(fdata->data, ioargs)) >= 0){
				if(fdata->auto_slide != 0)
					fdata->off += ioargs->buffer_size;
			}
			
			return ret;

		default:
			break;
	};
	return data_query(fdata->data, fargs);
} // }}}
static ssize_t data_slider_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	slider_t              *fdata;
	
	if(dst->ptr != NULL)
		return data_slider_t_handler(dst, (fastcall_header *)fargs);  // already inited - pass to underlying data
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			data_t                 data;
			uintmax_t              offset          = 0;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			hash_data_get(ret, TYPE_UINTT, offset, config, HK(offset));
			hash_holder_consume(ret, data, config, HK(data));
			if(ret != 0)
				return -EINVAL;
			
			if( (fdata = dst->ptr = slider_t_alloc(&data, offset, 1)) == NULL){
				data_free(&data);
				return -ENOMEM;
			}
			
			fdata->freeit = data;
			fdata->data   = &fdata->freeit;
			return 0;
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_slider_t_free(data_t *data, fastcall_free *fargs){ // {{{
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	slider_t_destroy(fdata);
	return 0;
} // }}}
static ssize_t data_slider_t_length(data_t *data, fastcall_length *fargs){ // {{{
	ssize_t                    ret;
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	switch(fargs->format){
		case FORMAT(native):
			if( (ret = data_query(fdata->data, fargs)) < 0)
				return ret;
			
			if( fdata->off > fargs->length ){
				fargs->length = 0;
				return ret;
			}
			
			fargs->length -= fdata->off;
			return ret;
			
		default: // we don't mess with other formats
			break;
		
	}
	return data_query(fdata->data, fargs);
} // }}}

data_proto_t slider_t_proto = {
	.type                   = TYPE_SLIDERT,
	.type_str               = "slider_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_PROXY,
	.handler_default        = (f_data_func)&data_slider_t_handler,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_slider_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_slider_t_free,
		[ACTION_LENGTH]       = (f_data_func)&data_slider_t_length,
		
		[ACTION_CONVERT_TO]   = (f_data_func)&data_default_convert_to,
		[ACTION_GETDATA]      = (f_data_func)&data_default_getdata,
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
	}
};
