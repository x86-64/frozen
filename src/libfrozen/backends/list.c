#include <libfrozen.h>
#include <alloca.h>
#define EMODULE 2

// 'list' chain provide insert capability using underlying chains.
// This is useful for upper-level chains like 'insert-sort'

static int lists_init(chain_t *chain){
	(void)chain;
	return 0;
}

static int lists_destroy(chain_t *chain){
	(void)chain;
	return 0;
}

static int lists_configure(chain_t *chain, hash_t *config){
	(void)chain; (void)config;
	return 0;
}

static ssize_t lists_set(chain_t *chain, request_t *request){
	ssize_t           ret;
	data_t           *key_orig;
	data_t            key_from, key_to;
	data_t            d_one = DATA_OFFT(1);
	
	if(hash_find(request, HK(insert)) != NULL){
		// on insert we move all items from 'key' to 'key'+1
		// recommended use of 'blocks' chain as under-lying chain to improve perfomance
		
		hash_data_find(request, HK(offset), &key_orig, NULL);
		if(key_orig == NULL)
			return warning("no offset supplied");
		
		data_copy_local(&key_from, key_orig);
		data_copy_local(&key_to,   key_orig);
			
		if(data_arithmetic('+', &key_to, NULL, &d_one, NULL) != 0) // TODO contexts
			return error("data_arithmetic failed");
		
		hash_t  new_request[] = {
			{ HK(action),   DATA_INT32(ACTION_CRWD_MOVE)               },
			{ HK(offset_from), key_from                                   },
			{ HK(offset_to),   key_to                                     },
			{ HK(size),     DATA_VOID                                  },
			hash_next(request)
		};
		
		ret = chain_next_query(chain, new_request); 
		if(ret != 0)
			return ret;
	}
	
	return chain_next_query(chain, request);
}

static ssize_t lists_delete(chain_t *chain, request_t *request){
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
		{ HK(action),      DATA_INT32(ACTION_CRWD_MOVE)               },
		{ HK(offset_from), key_from                                   },
		{ HK(offset_to),   key_to                                     },
		{ HK(size),        DATA_VOID                                  },
		hash_next(request)
	};
	
	ret = chain_next_query(chain, new_request);
	return ret;
}

static chain_t chain_lists = {
	"list",
	CHAIN_TYPE_CRWD,
	.func_init      = &lists_init,
	.func_configure = &lists_configure,
	.func_destroy   = &lists_destroy,
	{{
		.func_set    = &lists_set,
		.func_delete = &lists_delete
	}}
};
CHAIN_REGISTER(chain_lists)

