#include <libfrozen.h>
#include <alloca.h>
#define EMODULE 2

// 'list' backend provide insert capability using underlying backends.
// This is useful for upper-level backends like 'insert-sort'

static int lists_init(backend_t *backend){
	(void)backend;
	return 0;
}

static int lists_destroy(backend_t *backend){
	(void)backend;
	return 0;
}

static int lists_configure(backend_t *backend, hash_t *config){
	(void)backend; (void)config;
	return 0;
}

static ssize_t lists_set(backend_t *backend, request_t *request){
	ssize_t           ret;
	data_t           *key_orig;
	data_t            key_from, key_to;
	data_t            d_one = DATA_OFFT(1);
	
	if(hash_find(request, HK(insert)) != NULL){
		// on insert we move all items from 'key' to 'key'+1
		// recommended use of 'blocks' backend as under-lying backend to improve perfomance
		
		hash_data_find(request, HK(offset), &key_orig, NULL);
		if(key_orig == NULL)
			return warning("no offset supplied");
		
		data_copy_local(&key_from, key_orig);
		data_copy_local(&key_to,   key_orig);
			
		if(data_arithmetic('+', &key_to, NULL, &d_one, NULL) != 0) // TODO contexts
			return error("data_arithmetic failed");
		
		hash_t  new_request[] = {
			{ HK(action),   DATA_UINT32T(ACTION_CRWD_MOVE)               },
			{ HK(offset_from), key_from                                   },
			{ HK(offset_to),   key_to                                     },
			{ HK(size),     DATA_VOID                                  },
			hash_next(request)
		};
		
		ret = backend_pass(backend, new_request); 
		if(ret != 0)
			return ret;
	}
	
	return backend_pass(backend, request);
}

static ssize_t lists_delete(backend_t *backend, request_t *request){
	ssize_t           ret;
	hash_t           *key_orig;
	data_t           *key_data;
	data_t            key_from, key_to;
	size_t            r_size;
	
	if( (key_orig = hash_find(request, HK(offset))) == NULL)
		return warning("no offset supplied");

	key_data = hash_item_data(key_orig);
	hash_data_copy(ret, TYPE_SIZET, r_size, request, HK(size)); if(ret != 0) return warning("no size supplied");
	
	data_copy_local(&key_from, key_data);
	data_copy_local(&key_to,   key_data);
	
	data_t  d_size = DATA_OFFT(r_size);                           // TODO pass real data from hash
	if(data_arithmetic('+', &key_from, NULL, &d_size, NULL) != 0) // TODO contexts
		return error("data_arithmetic failed");
	
	hash_t  new_request[] = {
		{ HK(action),      DATA_UINT32T(ACTION_CRWD_MOVE)               },
		{ HK(offset_from), key_from                                   },
		{ HK(offset_to),   key_to                                     },
		{ HK(size),        DATA_VOID                                  },
		hash_next(request)
	};
	
	ret = backend_pass(backend, new_request);
	return ret;
}

backend_t backend_lists = {
	"list",
	.supported_api = API_CRWD,
	.func_init      = &lists_init,
	.func_configure = &lists_configure,
	.func_destroy   = &lists_destroy,
	{{
		.func_set    = &lists_set,
		.func_delete = &lists_delete
	}}
};


