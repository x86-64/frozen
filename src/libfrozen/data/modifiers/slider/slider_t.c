#include <libfrozen.h>
#include <slider_t.h>

#include <enum/format/format_t.h>
#include <core/hash/hash_t.h>
#include <numeric/uint/uint_t.h>

slider_t *        slider_t_alloc(data_t *data, uintmax_t offset){ // {{{
	slider_t                 *fdata;
	
	if( (fdata = malloc(sizeof(slider_t))) == NULL )
		return NULL;
	
	fdata->data       = data;
	fdata->off        = offset;
	fdata->frozen_off = ~0;
	
	data_set_void(&fdata->freeit);
	return fdata;
} // }}}
void              slider_t_destroy(slider_t *slider){ // {{{
	data_free(&slider->freeit);
} // }}}

uintmax_t data_slider_t_get_offset(data_t *data){ // {{{
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	return fdata->off;
} // }}}
void      data_slider_t_freeze(data_t *data){ // {{{
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	fdata->frozen_off = fdata->off;
} // }}}
void      data_slider_t_unfreeze(data_t *data){ // {{{
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	fdata->frozen_off = ~0;
} // }}}

static ssize_t data_slider_t_handler (data_t *data, fastcall_header *fargs){ // {{{
	size_t                     ret;
	slider_t                  *fdata             = (slider_t *)data->ptr;
	
	switch(fargs->action){
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io         *ioargs     = (fastcall_io *)fargs;
			
			if(fdata->frozen_off != ~0)
				ioargs->offset      += fdata->frozen_off;
			else
				ioargs->offset      += fdata->off;

			if( (ret = data_query(fdata->data, ioargs)) >= 0)
				fdata->off += ioargs->buffer_size;

			return ret;
		
		case ACTION_TRANSFER:
		case ACTION_CONVERT_TO:
		case ACTION_GETDATA:
			return data_protos[ TYPE_DEFAULTT ]->handlers[ fargs->action ](data, fargs);

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
			
			if( (fdata = dst->ptr = slider_t_alloc(&data, offset)) == NULL){
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
	.handler_default        = (f_data_func)&data_slider_t_handler,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_slider_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_slider_t_free,
		[ACTION_LENGTH]       = (f_data_func)&data_slider_t_length,
	}
};
