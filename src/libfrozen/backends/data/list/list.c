#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_list Backend 'data/list'
 */
/**
 * @ingroup mod_backend_list
 * @page page_list_info Description
 *
 * This backend simulate list of values. On writing item to <offset> it move all values from <offset> to <offset>+1.
 * On delete it do reverse operation.
 *
 * User request must have HK(insert) set for this.
 *
 * This module works in pair with "insert_sort" to provide clean inserts to file. This module move data on 1 (one) unit - use
 * "incapsulate" to move for valid length.
 */
/**
 * @ingroup mod_backend_list
 * @page page_list_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "data/list",
 * }
 * @endcode
 */

#define EMODULE 2

// 'list' backend provide insert capability using underlying backends.
// This is useful for upper-level backends like 'insert-sort'

static ssize_t lists_set(backend_t *backend, request_t *request){
	ssize_t           ret;
	off_t             from, to;
	
	if(hash_find(request, HK(insert)) != NULL){
		// on insert we move all items from 'key' to 'key'+1
		// recommended use of 'blocks' backend as under-lying backend to improve perfomance
		
		hash_data_copy(ret, TYPE_OFFT, from, request, HK(offset));
		if(ret != 0)
			return warning("no offset supplied");
		
		to = from + 1;
		
		hash_t  new_request[] = {
			{ HK(action),      DATA_UINT32T(ACTION_MOVE)             },
			{ HK(offset_from), DATA_PTR_OFFT(&from)                       },
			{ HK(offset_to),   DATA_PTR_OFFT(&to)                         },
			{ HK(size),        DATA_VOID                                  },
			hash_next(request)
		};
		
		if( (ret = backend_pass(backend, new_request)) < 0)
			return ret;
	}
	
	return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
}

static ssize_t lists_delete(backend_t *backend, request_t *request){
	ssize_t           ret;
	off_t             from, to;
	size_t            size;
	
	hash_data_copy(ret, TYPE_SIZET, size,   request, HK(size));   if(ret != 0) return warning("no size supplied");
	hash_data_copy(ret, TYPE_OFFT,  from,   request, HK(offset)); if(ret != 0) return warning("no offset supplied");
	
	to = from;
	from += size;
	
	hash_t  new_request[] = {
		{ HK(action),      DATA_UINT32T(ACTION_MOVE)             },
		{ HK(offset_from), DATA_PTR_OFFT(&from)                       },
		{ HK(offset_to),   DATA_PTR_OFFT(&to)                         },
		{ HK(size),        DATA_VOID                                  },
		hash_next(request)
	};
	return ( (ret = backend_pass(backend, new_request)) < 0) ? ret : -EEXIST;
}

backend_t list_proto = {
	.class          = "data/list",
	.supported_api  = API_CRWD,
	.backend_type_crwd = {
		.func_set    = &lists_set,
		.func_delete = &lists_delete
	}
};


