#include <libfrozen.h>

#define EMODULE 21

typedef struct lookup_userdata {
	uintmax_t              readonly;
	hash_key_t             output;
	hash_key_t             output_out;
	backend_t             *backend;
} lookup_userdata;

static int lookup_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(lookup_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int lookup_destroy(backend_t *backend){ // {{{
	lookup_userdata       *userdata = (lookup_userdata *)backend->userdata;

	free(userdata);
	return 0;
} // }}}
static int lookup_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              readonly          = 0;
	char                  *output_str        = NULL;
	char                  *output_out_str    = NULL;
	data_t                *backend_data      = NULL;
	lookup_userdata       *userdata          = (lookup_userdata *)backend->userdata;
	
	hash_data_find(config, HK(index), &backend_data, NULL);
	if(backend_data == NULL)
		return error("HK(index) not supplied");
	
	if( (userdata->backend = backend_from_data(backend_data)) == NULL)
		return error("supplied index backend not valid, or not found");
	
	hash_data_copy(ret, TYPE_STRINGT, output_str,         config, HK(output));
	hash_data_copy(ret, TYPE_STRINGT, output_out_str,     config, HK(output_out));
	hash_data_copy(ret, TYPE_UINTT,   readonly,           config, HK(readonly));
	
	userdata->readonly   = ( readonly == 0 ) ? 0 : 1;
	userdata->output     = hash_string_to_key(output_str);
	userdata->output_out = hash_string_to_key(output_out_str);
	return 0;
} // }}}

static ssize_t lookup_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                 d_void            = DATA_VOID;
	data_t                *d_output;
	data_t                *d_output_out;
	lookup_userdata       *userdata          = (lookup_userdata *)backend->userdata;
	
	request_t r_next[] = {
		{ userdata->output,      DATA_VOID },
		{ userdata->output_out,  DATA_VOID },
		hash_next(request)
	};
	if( (ret = backend_query(userdata->backend, r_next)) != 0)
		return ret;
	
	if( (ret = backend_pass(backend, r_next)) < 0)
		return ret;
	
	if(userdata->readonly == 0){ // can update if not readonly
		do{
			hash_data_find(r_next, userdata->output, &d_output_out, NULL);
		
			if( data_cmp(d_output, NULL, &d_void, NULL) == 0 )
				break;

			hash_data_find(r_next, userdata->output, &d_output,     NULL);
			
			if( data_cmp(d_output, NULL, d_output_out, NULL) == 0 )
				break;

			// data differs - need update
			request_t r_update[] = {
				{ userdata->output, *d_output_out },
				hash_next(request)
			};
			if( (ret = backend_query(userdata->backend, r_update)) != 0)
				return ret;
		}while(0);
	}
	return 0;
} // }}}

backend_t lookup_proto = {
	.class          = "index/lookup",
	.supported_api  = API_HASH,
	.func_init      = &lookup_init,
	.func_destroy   = &lookup_destroy,
	.func_configure = &lookup_configure,
	.backend_type_hash = {
		.func_handler = &lookup_handler
	}
};

