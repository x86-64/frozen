#include <libfrozen.h>
#include <dataproto.h>
#include <emitter_t.h>

#include <format/format_t.h>
#include <hash/hash_t.h>
#include <backend/backend_t.h>

static ssize_t data_emitter_t_handler (data_t *data, fastcall_header *hargs){ // {{{
	ssize_t                   ret;
	emitter_t                *fdata          = (emitter_t *)data->ptr;
	
	switch(hargs->action){
		case ACTION_CONVERT_FROM:;
			hash_t                   *parameters;
			fastcall_convert_from    *fargs            = (fastcall_convert_from *)hargs;
			
			data_get(ret, TYPE_HASHT, parameters, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			if(fdata == NULL){
				if( (data->ptr = fdata = calloc(1, sizeof(emitter_t))) == NULL)
					return -ENOMEM;

				fdata->allocated = 1;
			}
			
			hash_data_copy(ret, TYPE_BACKENDT, fdata->backend, parameters, HK(backend));
			hash_data_copy(ret, TYPE_HASHT,    fdata->request, parameters, HK(request));
			
			if(fdata->backend != NULL && fdata->request != NULL){
				fdata->request = hash_copy(fdata->request);
				break;
			}
			
			fdata->request = NULL;
			// fall
		case ACTION_FREE:
			if(fdata == NULL)
				return -EFAULT;
			
			if(fdata->backend)
				backend_destroy(fdata->backend);
			if(fdata->request)
				hash_free(fdata->request);
			if(fdata->allocated)
				free(data->ptr);
			
			data->ptr = NULL;
			break;
		
		default:
			if(fdata == NULL)
				return -EFAULT;
			
			return backend_query(fdata->backend, fdata->request);
	}
	return 0;
} // }}}

data_proto_t emitter_t_proto = {
	.type                   = TYPE_EMITTERT,
	.type_str               = "emitter_t",
	.api_type               = API_DEFAULT_HANDLER,
	.handler_default        = (f_data_func)&data_emitter_t_handler,
};
