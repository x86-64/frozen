#include <libfrozen.h>
#include <data_t.h>

#include <core/void/void_t.h>
#include <enum/format/format_t.h>
#include <modifiers/slider/slider_t.h>

static ssize_t data_data_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	datatype_t             datatype;
	data_t                 sl_input          = DATA_SLIDERT(fargs->src, 0);
	data_t                 d_datatype        = DATA_PTR_DATATYPET(&datatype);
	fastcall_convert_from  r_convert1        = { { 4, ACTION_CONVERT_FROM }, &sl_input, FORMAT(packed) };
	fastcall_convert_from  r_convert2        = { { 4, ACTION_CONVERT_FROM }, &sl_input, fargs->format };
	data_t                *fdata             = dst->ptr;
	
	if(dst->ptr == NULL)
		return -EINVAL;
	
	// get datatype
	if( (ret = data_query(&d_datatype, &r_convert1)) < 0)
		return ret;
	
	// accept datatype only eq void_t or datatype same as passed
	if(fdata->type == TYPE_VOIDT){
		fdata->type = datatype;
	}else if(fdata->type != datatype){
		return -EINVAL;
	}
	
	data_slider_t_freeze(&sl_input);
	
		ret = data_query(fdata, &r_convert2);
	
	data_slider_t_unfreeze(&sl_input);
	return ret;
} // }}}
static ssize_t data_data_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	data_t                *fdata              = src->ptr;
	data_t                 sl_output          = DATA_SLIDERT(fargs->dest, 0);
	data_t                 d_datatype         = DATA_PTR_DATATYPET(&fdata->type);
	fastcall_convert_to    r_convert1         = { { 4, ACTION_CONVERT_TO }, &sl_output, FORMAT(packed) };
	fastcall_convert_to    r_convert2         = { { 4, ACTION_CONVERT_TO }, &sl_output, fargs->format  };
	
	if(src->ptr == NULL)
		return -EINVAL;
	
	if( (ret = data_query(&d_datatype, &r_convert1)) < 0)
		return ret;
	
	data_slider_t_freeze(&sl_output);
		
		ret = data_query(fdata, &r_convert2);
		
	data_slider_t_unfreeze(&sl_output);
	return ret;
} // }}}

data_proto_t data_t_proto = {
	.type          = TYPE_DATAT,
	.type_str      = "data_t",
	.api_type      = API_HANDLERS,
	.handlers      = {
		[ACTION_CONVERT_TO]   = (f_data_func)&data_data_t_convert_to,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_data_t_convert_from,
	}
};

