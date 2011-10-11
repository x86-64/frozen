#include <libfrozen.h>

/**
 * @ingroup modules
 * @addtogroup mod_backend_lookup Module 'index/lookup'
 */
/**
 * @ingroup mod_backend_lookup
 * @page page_lookup_info Description
 *
 * This module query supplied index for key, and pass request to underlying backends with returned value
 */
/**
 * @ingroup mod_backend_lookup
 * @page page_lookup_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "index/lookup",
 *              output                  = "offset",           # output key for value
 *              output_type             = "uint_t",           # output data type 
 *              fatal                   = (uint_t)'1',        # queried entry in index must exist, default 0
 *              force_query             = (uint_t)'1',        # force query to index, even if HK( userdata->output ) already defined, default 0
 *              index                   = 
 *                                        "index_name",       # existing index
 *                                        { ... },            # new index configuration
 * }
 * @endcode
 */
/**
 * @ingroup mod_backend_lookup
 * @page page_lookup_io Input and output
 * 
 * First, query index with request. Next, pass request to underlying backends with returned value
 * 
 * @li If index return error - request stopped.
 * @li If index return empty result (no item found) - request passed with void data. (HK(fatal) can override it)
 *
 */

#define EMODULE 21

typedef struct lookup_userdata {
	uintmax_t              fatal;
	uintmax_t              force_query;
	backend_t             *backend_index;
	hash_key_t             output;
	data_type              output_type;
} lookup_userdata;

static int lookup_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(lookup_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int lookup_destroy(backend_t *backend){ // {{{
	lookup_userdata       *userdata = (lookup_userdata *)backend->userdata;
	
	if(userdata->backend_index != NULL)
		backend_destroy(userdata->backend_index);

	free(userdata);
	return 0;
} // }}}
static int lookup_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	char                  *output_str        = NULL;
	char                  *output_type_str   = NULL;
	lookup_userdata       *userdata          = (lookup_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT,  output_str,              config, HK(output));
	hash_data_copy(ret, TYPE_STRINGT,  output_type_str,         config, HK(output_type));
	if(ret != 0)
		return error("HK(output_type) not supplied");
	hash_data_copy(ret, TYPE_UINTT,    userdata->fatal,         config, HK(fatal));
	hash_data_copy(ret, TYPE_UINTT,    userdata->force_query,   config, HK(force_query));
	hash_data_copy(ret, TYPE_BACKENDT, userdata->backend_index, config, HK(index));
	if(ret != 0)
		return error("supplied index backend not valid, or not found");
	
	userdata->output      = hash_string_to_key(output_str);
	userdata->output_type = data_type_from_string(output_type_str);
	return 0;
} // }}}

static ssize_t lookup_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *d;
	data_t                 d_output;
	data_t                 d_void            = DATA_VOID;
	lookup_userdata       *userdata          = (lookup_userdata *)backend->userdata;
	
	if(
		userdata->force_query != 0 ||
		hash_find(request, userdata->output) == NULL
	){
		d_output.type = userdata->output_type;
		
		fastcall_alloc r_alloc = { { 3, ACTION_ALLOC }, 100 };
		if(data_query(&d_output, &r_alloc) < 0)
			return -ENOMEM;
		
		request_t r_query[] = {
			{ HK(action),          DATA_UINT32T(ACTION_READ) },
			{ userdata->output,    d_output                       },
			hash_next(request)
		};
		switch( (ret = backend_query(userdata->backend_index, r_query)) ){
			case 0:        d = &d_output; break;
			case -ENOENT:  d = &d_void;   break;
			default:       goto free;
		};
		
		request_t r_next[] = {
			{ userdata->output,   *d                              },
			hash_next(request)
		};
		ret = backend_pass(backend, r_next);
		
	free:;
		fastcall_alloc r_free = { { 2, ACTION_FREE } };
		data_query(&d_output, &r_free);
		
		return (ret < 0) ? ret : -EEXIST;
	}
	return ( (ret = backend_pass(backend, request)) < 0 ) ? ret : -EEXIST;
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

/*
	
	// NOTE Input rebuild request handles by following algo:
	//      1) check destination (current backend name), if not match - pass lower in chain
	//      2) pass request to index.  index in any case return ret code less than 0, so return triggered
	
	//hash_data_copy(ret, TYPE_STRINGT, destination, request, HK(destination));
	//if(ret == 0){
	//	if(strcmp(backend->name, destination) != 0)
	//		return ( (ret = backend_pass(userdata->backend_lookupend, request)) < 0) ? ret : -EEXIST;
	//}

	//userdata->backend_lookupend = backend_clone(backend);	
	//userdata->backend_lookupend->func_destroy                   = NULL;
	//userdata->backend_lookupend->backend_type_hash.func_handler = &lookupend_handler;
	
	//backend_connect(backend, userdata->backend_lookupend);
	//backend_insert(backend,  userdata->backend_index);
	if(userdata->readonly == 0){ // can update if not readonly
		do{
			hash_data_find(request, userdata->output, &d_output_out, NULL);
		
			if( data_cmp(d_output, NULL, &d_void, NULL) == 0 )
				break;

			hash_data_find(request, userdata->output, &d_output,     NULL);
			
			if( data_cmp(d_output, NULL, d_output_out, NULL) == 0 )
				break;

			// data differs - need update
			request_t r_update[] = {
				{ userdata->output, *d_output_out },
				hash_next(request)
			};
			if( (ret = backend_query(userdata->backend_index, r_update)) != 0)
				return ret;
		}while(0);
	}
*/
