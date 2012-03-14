#include <libfrozen.h>
#include <data_t.h>

#include <core/void/void_t.h>
#include <enum/format/format_t.h>
#include <modifiers/slider/slider_t.h>
#include <format/binstring/binstring_t.h>

static ssize_t data_data_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	data_t                *fdata             = dst->ptr;
	data_t                 sl_input          = DATA_SLIDERT(fargs->src, 0);
	
	if(fdata == NULL)
		return -EINVAL;
	
	fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
	if( (ret = data_query(fdata, &r_getdata)) < 0)
		return ret;
	
	fdata = r_getdata.data;
	
	data_t                 new_data          = DATA_VOID;
	data_t                 d_datatype        = DATA_PTR_DATATYPET(&new_data.type);
	data_t                 d_binstring       = DATA_BINSTRINGT(&new_data);
	fastcall_convert_from  r_convert         = { { 4, ACTION_CONVERT_FROM }, &sl_input, FORMAT(packed) };
	// get datatype
	if( (ret = data_query(&d_datatype, &r_convert)) < 0)
		return ret;
	
	// get data
	data_slider_t_freeze(&sl_input);
		
		if( data_is_greedy(&new_data) ){
			ret = data_query(&d_binstring, &r_convert);
		}else{
			ret = data_query(&new_data, &r_convert);
		}
	
	data_slider_t_unfreeze(&sl_input);
	
	// accept data only eq void_t or datatype same as passed
	if(fdata->type == TYPE_VOIDT || fdata->type == new_data.type){
		*fdata = new_data;
		return ret;
	}
	return -EINVAL;
} // }}}
static ssize_t data_data_t_convert_to(data_t *src, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	data_t                 d_void             = DATA_VOID;
	data_t                *fdata              = src->ptr;
	data_t                 sl_output          = DATA_SLIDERT(fargs->dest, 0);
	
	if(src->ptr == NULL)
		return -EINVAL;
	
	fastcall_getdata r_getdata = { { 3, ACTION_GETDATA } };
	if( (ret = data_query(fdata, &r_getdata)) < 0)
		return ret;
	
	fdata = r_getdata.data;
	
	if(fdata->type == TYPE_MACHINET){ // HACK HACK HACK
		fdata = &d_void;
	}
	
	data_t                 d_datatype         = DATA_PTR_DATATYPET(&fdata->type);
	data_t                 d_binstring        = DATA_BINSTRINGT(fdata);
	fastcall_convert_to    r_convert          = { { 4, ACTION_CONVERT_TO }, &sl_output, FORMAT(packed) };
	
	// write datatype
	if( (ret = data_query(&d_datatype, &r_convert)) < 0)
		return ret;
	
	// write data
	data_slider_t_freeze(&sl_output);
		
		if( data_is_greedy(fdata) ){
			ret = data_query(&d_binstring, &r_convert);
		}else{
			ret = data_query(fdata, &r_convert);
		}
		
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

