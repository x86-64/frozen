#include <libfrozen.h>

typedef struct lists_user_data {
	request_t   *move_request;
	buffer_t    *move_buffer;
} lists_user_data;

// 'list' chain provide insert capability using underlying chains.
// This is useful for upper-level chains like 'insert-sort'

static int lists_init(chain_t *chain){
	unsigned int     action;
	lists_user_data *user_data = calloc(1, sizeof(lists_user_data));
	if(user_data == NULL)
		return -ENOMEM;
	
	chain->user_data = user_data;
	
	user_data->move_request = hash_new();
	user_data->move_buffer  = buffer_alloc();
	
	action = ACTION_CRWD_MOVE;
	hash_set(user_data->move_request, "action", TYPE_INT32, &action);
	
	return 0;
}

static int lists_destroy(chain_t *chain){
	lists_user_data *data = (lists_user_data *)chain->user_data;
	
	buffer_free(data->move_buffer);
	hash_free(data->move_request);
	
	free(chain->user_data);
	chain->user_data = NULL;
	return 0;
}

static int lists_configure(chain_t *chain, setting_t *config){
	return 0;
}

static ssize_t lists_rest(chain_t *chain, request_t *request, buffer_t *buffer){
	return chain_next_query(chain, request, buffer);
}

static ssize_t lists_set(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t           ret;
	unsigned int      insert;
	data_t           *key;
	data_type         key_type;
	data_proto_t     *key_proto;
	lists_user_data  *data = (lists_user_data *)chain->user_data;
	
	if(
		hash_get_copy(request, "insert", TYPE_INT32, &insert, sizeof(insert)) == 0 &&
		insert == 1
	){
		// on insert we move all items from 'key' to 'key'+1
		// recommended use of 'blocks' chain as under-lying chain to improve perfomance
		
		if(hash_get_any(request, "key", &key_type, &key, NULL) != 0)
			return -EINVAL;
		
		hash_set(data->move_request, "key_from", key_type, key);
		hash_set(data->move_request, "key_to",   key_type, key);
		
		if(hash_get_any(data->move_request, "key_to", &key_type, &key, NULL) != 0)
			return -EINVAL;
		
		key_proto = data_proto_from_type(key_type);
		if(key_proto->func_add(key, 1) != 0)
			return -EINVAL;
		
		ret = chain_next_query(chain, data->move_request, data->move_buffer); 
		if(ret != 0)
			return ret;
	}
	
	return chain_next_query(chain, request, buffer);
}

static ssize_t lists_delete(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t           ret;
	data_t           *key;
	data_type         key_type;
	data_proto_t     *key_proto;
	lists_user_data  *data = (lists_user_data *)chain->user_data;
	
	if(hash_get_any(request, "key", &key_type, &key, NULL) != 0)
		return -EINVAL;
		
	hash_set(data->move_request, "key_from", key_type, key);
	hash_set(data->move_request, "key_to",   key_type, key);
	
	if(hash_get_any(data->move_request, "key_from", &key_type, &key, NULL) != 0)
		return -EINVAL;
	
	key_proto = data_proto_from_type(key_type);
	if(key_proto->func_add(key, 1) != 0)
		return -EINVAL;
	
	hash_dump(data->move_request);
	ret = chain_next_query(chain, data->move_request, data->move_buffer); 
	
	return ret;
}

static chain_t chain_lists = {
	"list",
	CHAIN_TYPE_CRWD,
	&lists_init,
	&lists_configure,
	&lists_destroy,
	{{
		.func_create = &lists_rest,
		.func_set    = &lists_set,
		.func_get    = &lists_rest,
		.func_delete = &lists_delete,
		.func_move   = &lists_rest,
		.func_count  = &lists_rest
	}}
};
CHAIN_REGISTER(chain_lists)

