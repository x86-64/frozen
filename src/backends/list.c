#include <libfrozen.h>

// 'list' chain provide insert capability using underlying chains.
// This is useful for upper-level chains like 'insert-sort'

static int lists_init(chain_t *chain){
	return 0;
}

static int lists_destroy(chain_t *chain){
	return 0;
}

static int lists_configure(chain_t *chain, hash_t *config){
	return 0;
}

static ssize_t lists_set(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t           ret;
	hash_t           *r_insert, *r_key;
	void             *o_key_from, *o_key_to;
	
	if(
		(r_insert = hash_find_typed(request, TYPE_INT32, "insert")) != NULL &&
		HVALUE(r_insert, unsigned int) == 1
	){
		// on insert we move all items from 'key' to 'key'+1
		// recommended use of 'blocks' chain as under-lying chain to improve perfomance
		
		if( (r_key = hash_find(request, "key")) == NULL) return -EINVAL;
			
		o_key_from = alloca(r_key->value_size);
		o_key_to   = alloca(r_key->value_size);
			
		memcpy(o_key_from, r_key->value, r_key->value_size);
		memcpy(o_key_to,   r_key->value, r_key->value_size);
			
		if(data_bare_arithmetic(r_key->type, o_key_to, '+', 1) != 0) return -EINVAL;
		
		hash_t  new_request[] = {
			{ "action",   DATA_INT32(ACTION_CRWD_MOVE)               },
			{ "key_from", r_key->type, o_key_from, r_key->value_size },
			{ "key_to",   r_key->type, o_key_to,   r_key->value_size },
			{ "size",     DATA_VOID                                  },
			hash_next(request)
		};
		
		ret = chain_next_query(chain, new_request, buffer); 
		if(ret != 0)
			return ret;
	}
	
	return chain_next_query(chain, request, buffer);
}

static ssize_t lists_delete(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t           ret;
	hash_t           *r_key, *r_size;
	void             *o_key_from, *o_key_to;
	
	if( (r_key  = hash_find(request, "key"))  == NULL) return -EINVAL;
	if( (r_size = hash_find(request, "size")) == NULL) return -EINVAL;
		
	o_key_from = alloca(r_key->value_size);
	o_key_to   = alloca(r_key->value_size);
		
	memcpy(o_key_from, r_key->value, r_key->value_size);
	memcpy(o_key_to,   r_key->value, r_key->value_size);
		
	if(data_bare_arithmetic(r_key->type, o_key_from, '+', HVALUE(r_size, unsigned int)) != 0) return -EINVAL;
	
	hash_t  new_request[] = {
		{ "action",   DATA_INT32(ACTION_CRWD_MOVE)               },
		{ "key_from", r_key->type, o_key_from, r_key->value_size },
		{ "key_to",   r_key->type, o_key_to,   r_key->value_size },
		{ "size",     DATA_VOID                                  },
		hash_next(request)
	};
	
	ret = chain_next_query(chain, new_request, buffer); 
	return ret;
}

static chain_t chain_lists = {
	"list",
	CHAIN_TYPE_CRWD,
	&lists_init,
	&lists_configure,
	&lists_destroy,
	{{
		.func_set    = &lists_set,
		.func_delete = &lists_delete
	}}
};
CHAIN_REGISTER(chain_lists)

