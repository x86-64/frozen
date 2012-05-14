#include <libfrozen.h>
#include <consumable_t.h>

ssize_t        data_consumable_t(data_t *data, data_t child){ // {{{
	consumable_t          *fdata;
	
	data->type = TYPE_CONSUMABLET;
	if( (data->ptr = fdata = malloc(sizeof(consumable_t))) == NULL)
		return -ENOMEM;
	
	fdata->data           = child;
	fdata->not_consumable = 0;
	fdata->heap           = 1;
	return 0;
} // }}}
ssize_t        data_notconsumable_t(data_t *data, data_t child){ // {{{
	ssize_t                ret;
	
	if( (ret = data_consumable_t(data, child)) < 0)
		return ret;
	
	consumable_t          *fdata             = (consumable_t *)data->ptr;
	fdata->not_consumable = 1;
	return 0;
} // }}}

static ssize_t data_consumable_t_handler(data_t *data, fastcall_header *hargs){ // {{{
	consumable_t          *fdata             = (consumable_t *)data->ptr;
	
	return data_query(&fdata->data, hargs);
} // }}}
static ssize_t data_consumable_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	consumable_t          *fdata             = (consumable_t *)data->ptr;
	
	if(fdata != NULL)
		return data_consumable_t_handler(data, (fastcall_header *)fargs);
	
	switch(fargs->format){
		case FORMAT(native):;
			data_t                     d_copy;
			consumable_t              *fargs_src        = (consumable_t *)fargs->src->ptr;
			
			if(data->type != fargs->src->type)
				return -EINVAL;
			
			holder_copy(ret, &d_copy, &fargs_src->data);
			if(ret != 0)
				return ret;
			
			return data_consumable_t(data, d_copy);
	};
	return -ENOSYS;
} // }}}
static ssize_t data_consumable_t_free(data_t *data, fastcall_free *fargs){ // {{{
	consumable_t          *fdata             = (consumable_t *)data->ptr;
	
	if(!fdata)
		return 0;
	
	data_consumable_t_handler(data, (fastcall_header *)fargs);
	
	if(fdata->heap == 1)
		free(fdata);
	data_set_void(data);
	return 0;
} // }}}
static ssize_t data_consumable_t_consume(data_t *data, fastcall_consume *fargs){ // {{{
	ssize_t                ret;
	consumable_t          *fdata             = (consumable_t *)data->ptr;
	
	if(fdata->not_consumable == 0){
		*fargs->dest = fdata->data;
		data_set_void(&fdata->data);
		ret = 0;
	}else{
		holder_copy(ret, fargs->dest, &fdata->data);
	}
	return ret;
} // }}}

data_proto_t consumable_t_proto = {
	.type_str               = "consumable_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_PROXY,
	.handler_default        = (f_data_func)&data_consumable_t_handler,
	.handlers               = {
		[ACTION_CONVERT_FROM]  = (f_data_func)&data_consumable_t_convert_from,
		[ACTION_FREE]          = (f_data_func)&data_consumable_t_free,
		[ACTION_CONSUME]       = (f_data_func)&data_consumable_t_consume,
	}
};
