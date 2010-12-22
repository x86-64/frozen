#include <libfrozen.h>
#include <alloca.h>

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
	
	if(hash_find(request, "insert") != NULL){
		// on insert we move all items from 'key' to 'key'+1
		// recommended use of 'blocks' chain as under-lying chain to improve perfomance
		
		if( (key_orig = hash_get_data(request, "key")) == NULL)
			return -EINVAL;
		
		data_copy_local(&key_from, key_orig);
		data_copy_local(&key_to,   key_orig);
			
		if(data_arithmetic('+', &key_to, NULL, &d_one, NULL) != 0) // TODO contexts
			return -EINVAL;
		
		hash_t  new_request[] = {
			{ "action",   DATA_INT32(ACTION_CRWD_MOVE)               },
			{ "key_from", key_from                                   },
			{ "key_to",   key_to                                     },
			{ "size",     DATA_VOID                                  },
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
	data_t           *key_orig;
	data_t            key_from, key_to;
	hash_t           *r_size;
	
	if( (key_orig = hash_get_data(request, "key"))  == NULL) return -EINVAL;
	if( (r_size   = hash_find    (request, "size")) == NULL) return -EINVAL;
	
	data_copy_local(&key_from, key_orig);
	data_copy_local(&key_to,   key_orig);
	
	data_t  d_size = DATA_OFFT(HVALUE(r_size, unsigned int));     // TODO pass real data from hash
	if(data_arithmetic('+', &key_from, NULL, &d_size, NULL) != 0) // TODO contexts
		return -EINVAL;
	
	hash_t  new_request[] = {
		{ "action",   DATA_INT32(ACTION_CRWD_MOVE)               },
		{ "key_from", key_from                                   },
		{ "key_to",   key_to                                     },
		{ "size",     DATA_VOID                                  },
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

