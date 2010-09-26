#include <libfrozen.h>

#define BLOCKS_DBG

typedef struct block_info {
	unsigned int  real_block_off; 
	unsigned int  size;
} block_info;

typedef struct blocks_user_data {
	crwd_fastcall fc_table;
	size_t        block_size;
	off_t         blocks_count;
	backend_t    *backend;
	request_t    *layer_count;
	request_t    *layer_get;

} blocks_user_data;

// 'blocks' chain splits underlying data space into blocks.
// This is useful for reducing copy time in insert operations

// TODO recheck all buffer_read_flat, to init variables

static ssize_t   map_new            (blocks_user_data *data, unsigned int b_rid, unsigned int b_size){ // {{{
	int      action;
	
	action = ACTION_CRWD_WRITE;
	hash_set(data->fc_table.request_create, "action",     TYPE_INT32, &action);
	hash_set(data->fc_table.request_create, "block_off",  TYPE_INT32, &b_rid);
	hash_set(data->fc_table.request_create, "block_size", TYPE_INT32, &b_size);
	
	return backend_query(data->backend, data->fc_table.request_create, NULL);
} // }}}
static ssize_t   map_insert         (blocks_user_data *data, unsigned int b_vid, unsigned int b_rid, unsigned int b_size){ // {{{
	int      action;
	
	action = ACTION_CRWD_WRITE;
	hash_set(data->fc_table.request_create, "action",     TYPE_INT32, &action);
	hash_set(data->fc_table.request_create, "block_vid",  TYPE_INT32, &b_vid);
	hash_set(data->fc_table.request_create, "block_off",  TYPE_INT32, &b_rid);
	hash_set(data->fc_table.request_create, "block_size", TYPE_INT32, &b_size);
	
	return backend_query(data->backend, data->fc_table.request_create, NULL);
} // }}}
static ssize_t   map_resize         (blocks_user_data *data, unsigned int b_vid, unsigned int new_size){ // {{{
	hash_set(data->fc_table.request_write, "block_vid",  TYPE_INT32, &b_vid);
	hash_set(data->fc_table.request_write, "block_size", TYPE_INT32, &new_size);
	
	return backend_query(data->backend, data->fc_table.request_write, NULL);
} // }}}
static ssize_t   map_delete         (blocks_user_data *data, unsigned int b_vid){ // {{{
	hash_set(data->fc_table.request_delete, "block_vid", TYPE_INT32, &b_vid);
	
	return backend_query(data->backend, data->fc_table.request_delete, NULL);
} // }}}
static ssize_t   map_off            (blocks_user_data *data, off_t virt_off, off_t *real_off){ // {{{
	ssize_t  ret;
	
	hash_set(data->fc_table.request_read, "offset", TYPE_INT64, &virt_off);
	
	ret = backend_query(data->backend, data->fc_table.request_read, data->fc_table.buffer_read);
	if(ret > 0){
		buffer_read_flat(data->fc_table.buffer_read, ret, real_off, sizeof(off_t));
	}
	return ret;
} // }}}
static ssize_t   map_get_block_size (blocks_user_data *data, unsigned int block_vid, size_t *block_size){ // {{{
	int      action;
	ssize_t  ret     = 1;
	
	action = ACTION_CRWD_READ;
	hash_set(data->fc_table.request_move, "action",    TYPE_INT32, &action);
	hash_set(data->fc_table.request_move, "blocks",    TYPE_INT32, &ret);
	hash_set(data->fc_table.request_move, "block_vid", TYPE_INT32, &block_vid);
	
	ret = backend_query(data->backend, data->fc_table.request_move, data->fc_table.buffer_create);
	if(ret > 0){
		buffer_read_flat(data->fc_table.buffer_create, ret, block_size, sizeof(size_t));
	}
	return ret;
} // }}}
static ssize_t   map_blocks         (blocks_user_data *data){ // {{{
	ssize_t  ret = 1;
	
	data->blocks_count = 0;
	
	hash_set(data->fc_table.request_count, "blocks", TYPE_INT32, &ret);
	
	ret = backend_query(data->backend, data->fc_table.request_count, data->fc_table.buffer_count);
	if(ret > 0){
		buffer_read_flat(data->fc_table.buffer_count, ret, &data->blocks_count, sizeof(data->blocks_count));
	}
	return ret;
} // }}}
static ssize_t   real_new           (chain_t *chain, request_t *request, off_t *real_block_id){ // {{{
	ssize_t           ret;
	off_t             block_id;
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	block_id = 0;
	
	hash_empty        (data->layer_get);
	hash_assign_layer (data->layer_get, request);
	hash_set          (data->layer_get, "size", TYPE_INT32, &data->block_size);
	
	ret = chain_next_query(chain, data->layer_get, data->fc_table.buffer_create);
	if(ret <= 0)
		return -EINVAL;
	buffer_read_flat(data->fc_table.buffer_create, ret, &block_id, sizeof(block_id));
	
	block_id = block_id / data->block_size;
	
	*real_block_id = block_id;
	
	return 0;
} // }}}
static ssize_t   map_translate_key  (blocks_user_data *data, char *key_name, request_t *request, request_t *layer){ // {{{
	off_t        key_virt, key_real;
	// TODO remove TYPE_INT64 from code, support all
	if(hash_get_copy (request, key_name, TYPE_INT64, &key_virt, sizeof(key_virt)) != 0)
		return -EINVAL;
	
	if(map_off(data, key_virt, &key_real) <= 0)
		return -EINVAL;
	
	hash_set(layer, key_name, TYPE_INT64, &key_real);
	
	return 0;
} // }}}




/* init {{{ */
static int blocks_init(chain_t *chain){
	blocks_user_data *user_data = calloc(1, sizeof(blocks_user_data));
	if(user_data == NULL)
		return -ENOMEM;
	
	chain->user_data = user_data;
	
	return 0;
}

static int blocks_destroy(chain_t *chain){
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	hash_free(data->layer_get);
	hash_free(data->layer_count);
	fc_crwd_destory(&data->fc_table);
	backend_destory(data->backend);
	free(chain->user_data);
	
	chain->user_data = NULL;
	return 0;
}

static int blocks_configure(chain_t *chain, setting_t *config){
	setting_t        *config_backend;
	char             *block_size_str;
	unsigned int      action;
	blocks_user_data *data         = (blocks_user_data *)chain->user_data;
	
	if( (block_size_str = setting_get_child_string(config, "block_size")) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'block_size' not set\n");
	
	if( (data->block_size = strtoul(block_size_str, NULL, 10)) == 0)
		return_error(-EINVAL, "chain 'blocks' variable 'block_size' invalid\n");
	
	if( (config_backend = setting_get_child_setting(config, "backend")) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'backend' not set\n");
	
	if( (data->backend = backend_new("blocks_map", config_backend)) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'backend' invalid\n");
	
	if( fc_crwd_init(&data->fc_table) != 0){
		error("chain 'blocks' fastcall table init failed\n"); goto free;
	}
	
	if( (data->layer_get   = hash_new()) == NULL)
		goto free1;
	
	if( (data->layer_count = hash_new()) == NULL)
		goto free2;
	
	action = ACTION_CRWD_COUNT;
	hash_set(data->layer_count, "action", TYPE_INT32, &action);
	
	map_blocks(data);
	
	return 0;

free2:  hash_free(data->layer_get);
free1:  fc_crwd_destory(&data->fc_table);
free:	backend_destory(data->backend);
	return -ENOMEM;
}
/* }}} */

static ssize_t blocks_create(chain_t *chain, request_t *request, buffer_t *buffer){
	unsigned int      element_size;
	
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	size_t            block_size;
	off_t             block_id;
	ssize_t           ret;
	
	
	if(hash_get_copy (request, "size", TYPE_INT32, &element_size, sizeof(element_size)) != 0)
		return -EINVAL;
	
	if(element_size > data->block_size) // TODO make sticked blocks
		return -EINVAL;
	/* if file empty - create new block */
	if(data->blocks_count == 0){
		if(real_new(chain, request, &block_id) != 0)
			return -EFAULT;
		
		if(map_new(data, block_id * data->block_size, 0) != 0)
			return -EFAULT;
		
		if(map_blocks(data) <= 0)
			return -EFAULT;
	}
	
	/* try write into last block */
	block_id = data->blocks_count - 1;
	if(map_get_block_size(data, block_id, &block_size) <= 0)
		return -EFAULT;
	
	/* if not enough space - create new one */
	if(element_size + block_size > data->block_size){
		if(real_new(chain, request, &block_id) != 0)
			return -EFAULT;
		
		if(map_new(data, block_id * data->block_size, 0) != 0)
			return -EFAULT;
		
		if(map_blocks(data) <= 0)
			return -EFAULT;
		
		block_size = 0;
	}
	
	/* calc virt_ptr */
	hash_assign_layer  (data->layer_count, request);               // transfer query to map backend _COUNT
	ret = backend_query(data->backend, data->layer_count, buffer); 
	
	/* really add item */
	if(map_resize(data, block_id, block_size + element_size) != 0)
		return -EFAULT;
	
	return ret;
}

static ssize_t blocks_setget(chain_t *chain, request_t *request, buffer_t *buffer){
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	hash_empty        (data->layer_get);
	hash_assign_layer (data->layer_get, request);
	
	if(map_translate_key(data, "key", request, data->layer_get) != 0)
		return -EINVAL;
	
	return chain_next_query(chain, data->layer_get, buffer);
}

static ssize_t blocks_delete(chain_t *chain, request_t *request, buffer_t *buffer){
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	map_delete(data, 0);
	map_insert(data, 0, 0, 0);
	return -1;
}

static ssize_t blocks_move(chain_t *chain, request_t *request, buffer_t *buffer){
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	hash_empty        (data->layer_get);
	hash_assign_layer (data->layer_get, request);
	
	//if(map_translate_key(data, "key_from", request) != 0)
	//	return -EINVAL;
	//if(map_translate_key(data, "key_to",   request) != 0)
	//	return -EINVAL;
	
	return chain_next_query(chain, data->layer_get, buffer);
}

static ssize_t blocks_count(chain_t *chain, request_t *request, buffer_t *buffer){
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	hash_assign_layer   (data->layer_count, request);               // transfer query to map backend _COUNT
	return backend_query(data->backend, data->layer_count, buffer); 
}

static chain_t chain_blocks = {
	"blocks",
	CHAIN_TYPE_CRWD,
	&blocks_init,
	&blocks_configure,
	&blocks_destroy,
	{{
		.func_create = &blocks_create,
		.func_set    = &blocks_setget,
		.func_get    = &blocks_setget,
		.func_delete = &blocks_delete,
		.func_move   = &blocks_move,
		.func_count  = &blocks_count
	}}
};
CHAIN_REGISTER(chain_blocks)

