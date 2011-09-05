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
 *              output_out              = "offset_out",       # output_out key for value, it changed from underlying backends
 *              readonly                = (uint_t)'1',        # forbid writes and updates to index, default 0
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
 * First, query supplied index with request. Next, pass request to underlying backends with returned value
 * 
 * @li If index return error - request stopped.
 * @li If after passing request <output> and <output_out> data differs - module will update index.
 *
 * Input rebuild request handles by following algo:
 * @li check destination (current backend name), if not match - pass lower in chain
 * @li pass request to index. index in any case return ret code less than 0, so return triggered
 *
 */
/**
 * @ingroup mod_backend_lookup
 * @page page_lookup_overview Architecture
 * 
 * Most modules follow "chain" structure, then module do something with input request and pass it to next
 * module in chain. This module differs. It query "index" and pass special variable HK(pass_to) to it.
 * Doing so, "index" module then passing request, use this variable and request flow to next module in "lookup" module chain.
 * Advantages of this: first, all data handling moved to "index" module, where it belongs, so data can be
 * stored in stack and don't use malloc() at all. And second, such structure hide "index" from main stream of requests,
 * allowing HK(destination) and HK(readonly) work properly.
 */

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
	char                  *destination       = NULL;
	data_t                 d_void            = DATA_VOID;
	data_t                *d_output;
	data_t                *d_output_out;
	lookup_userdata       *userdata          = (lookup_userdata *)backend->userdata;
	
	// NOTE Input rebuild request handles by following algo:
	//      1) check destination (current backend name), if not match - pass lower in chain
	//      2) pass request to index.  index in any case return ret code less than 0, so return triggered
	
	hash_data_copy(ret, TYPE_STRINGT, destination, request, HK(destination));
	if(ret == 0){
		if(strcmp(backend->name, destination) != 0)
			return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	}

	request_t r_next[] = {
		{ HK(pass_to),           DATA_BACKENDT(backend) },
		{ userdata->output,      DATA_VOID              },
		{ userdata->output_out,  DATA_VOID              },
		hash_next(request)
	};
	if( (ret = backend_query(userdata->backend, r_next)) != 0)
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

