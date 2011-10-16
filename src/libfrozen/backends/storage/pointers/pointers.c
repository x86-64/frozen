#include <libfrozen.h>

/**
 * @pointers pointers.c
 * @ingroup modules
 * @brief Pointers module
 *
 * Pointers module store pointers to data instead of actual data
 */
/**
 * @ingroup modules
 * @addtogroup mod_pointers Module 'pointers'
 */
/**
 * @ingroup mod_pointers
 * @page page_pointers_config Configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class    = "pointers",
 *              input    = "buffer",             # input data HK
 *              offset   = "offset",             # offset HK
 * 	}
 * @endcode
 */

#define EMODULE         32

typedef struct pointers_userdata {
	hash_key_t             input;
	hash_key_t             offset;
} pointers_userdata;

static int pointers_init(backend_t *backend){ // {{{
	pointers_userdata *userdata = calloc(1, sizeof(pointers_userdata));
	if(userdata == NULL)
		return error("calloc returns null");
	
	backend->userdata = userdata;
	
	userdata->input = HK(buffer);
	userdata->offset = HK(offset);
	return 0;
} // }}}
static int pointers_destroy(backend_t *backend){ // {{{
	pointers_userdata *userdata = (pointers_userdata *)backend->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int pointers_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	pointers_userdata     *userdata          = (pointers_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->input,  config, HK(input));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->offset, config, HK(offset));
	return 0;
} // }}}

static ssize_t pointers_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uint32_t               action;
	uintmax_t              offset;
	data_t                *input;
	data_t                 d_void            = DATA_VOID;
	pointers_userdata     *userdata          = (pointers_userdata *)backend->userdata;

	hash_data_copy(ret, TYPE_UINT32T, action, request, HK(action));
	if(ret != 0) return -ENOSYS;
	
	hash_data_copy(ret, TYPE_UINTT,   offset, request, userdata->offset);
	if(ret != 0) return -EINVAL; // TODO remove for ACTION_CREATE
	
	offset *= sizeof(data_t);
	
	input = hash_data_find(request, userdata->input);
	
	switch(action){
		// TODO ACTION_CREATE
		case ACTION_DELETE:
			input = &d_void;
		
		case ACTION_READ:;
		case ACTION_WRITE:;
			request_t r_next[] = {
				{ userdata->offset, DATA_PTR_UINTT(&offset)         },
				{ userdata->input,  DATA_RAW(input, sizeof(data_t)) },
				{ HK(size),         DATA_SIZET(sizeof(data_t))      }, // TODO remove size
				hash_next(request)
			};
			return ( (ret = backend_pass(backend, r_next)) < 0) ? ret : -EEXIST;
	};
	return 0;
} // }}}

backend_t pointers_proto = {
	.class          = "storage/pointers",
	.supported_api  = API_HASH,
	.func_init      = &pointers_init,
	.func_configure = &pointers_configure,
	.func_destroy   = &pointers_destroy,
	.backend_type_hash = {
		.func_handler = &pointers_handler,
	}
};

