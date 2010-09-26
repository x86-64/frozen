#include <libfrozen.h>

typedef struct block_info {
	unsigned int real_block_id; 
	unsigned int free_size;
} block_info;

typedef struct blocks_user_data {
	size_t      block_size;
	off_t       blocks_count;
	backend_t  *backend;
	request_t  *request_read;
	request_t  *request_write;
	data_t     *request_read_key;
	data_t     *request_write_key;
	buffer_t   *buffer_read;
	
} blocks_user_data;

typedef struct block_t {
	size_t      free_size;
	off_t       real_off;
} block_t;

// 'blocks' chain splits underlying data space into blocks.
// This is useful for reducing copy time in insert operations

/* private {{{ */
static void    map_get_blocks_count(blocks_user_data *data){
	unsigned int   ret;
	unsigned int   action;
	request_t     *request;
	buffer_t      *buffer;
	off_t          blocks_count = 0;
	
	request = hash_new();
	buffer  = buffer_alloc();
	
	action  = ACTION_CRWD_COUNT;
	hash_set(request, "action", TYPE_INT32, &action);
	
	ret = backend_query(data->backend, request, buffer);
	if(ret > 0){
		buffer_read_flat(buffer, ret, &blocks_count, sizeof(blocks_count));
	}
	
	data->blocks_count = blocks_count;
	
	hash_free(request);
	buffer_free(buffer);
}

static int     map_get_block(blocks_user_data *data, off_t block_id, block_t *block){
	unsigned int  ret;
	block_info    info;
	
	if(block_id >= data->blocks_count || block == NULL)
		return -EINVAL;
	
	*(off_t *)(data->request_read_key) = block_id;
	ret = backend_query(data->backend, data->request_read, data->buffer_read);
	if(ret <= 0 || ret < sizeof(info))
		return -EINVAL;
	
	buffer_read_flat(data->buffer_read, ret, &info, sizeof(info));
	
	if(info.free_size > data->block_size || info.real_block_id >= data->blocks_count)
		return -EINVAL;
	
	block->free_size = info.free_size;
	block->real_off  = info.real_block_id * data->block_size;
	
	return 0;
}

/*
static off_t   map_translate_key(blocks_user_data *data, off_t vm_off){
	return -1;
}*/

/* }}} */
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
	
	backend_destory(data->backend);
	free(chain->user_data);
	
	chain->user_data = NULL;
	return 0;
}

static int blocks_configure(chain_t *chain, setting_t *config){
	setting_t        *config_backend;
	char             *block_size_str;
	unsigned int      action;
	off_t             key;
	size_t            size;
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	if( (block_size_str = setting_get_child_string(config, "block_size")) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'block_size' not set");
	
	if( (data->block_size = strtoul(block_size_str, NULL, 10)) == 0)
		return_error(-EINVAL, "chain 'blocks' variable 'block_size' invalid");
	
	if( (config_backend = setting_get_child_setting(config, "backend")) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'backend' not set");
	
	if( (data->backend = backend_new("blocks_map", config_backend)) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'backend' invalid");
	
	if( (data->request_read  = hash_new()) == NULL)
		goto free;
	
	if( (data->request_write = hash_new()) == NULL)
		goto free2;
	
	if( (data->buffer_read = buffer_alloc()) == NULL)
		goto free3;
	
	action = ACTION_CRWD_READ;	
	hash_set(data->request_read,  "action", TYPE_INT32, &action);
	action = ACTION_CRWD_WRITE;
	hash_set(data->request_write, "action", TYPE_INT32, &action);
	
	key    = 0;
	hash_set(data->request_read,  "key",    TYPE_INT64, &key);
	hash_set(data->request_write, "key",    TYPE_INT64, &key);
	hash_get_any(data->request_read,  "key", NULL, &data->request_read_key,  NULL);
	hash_get_any(data->request_write, "key", NULL, &data->request_write_key, NULL);
	
	size   = 1;
	hash_set(data->request_read,  "size",   TYPE_INT32, &size);
	hash_set(data->request_write, "size",   TYPE_INT32, &size);
	
	map_get_blocks_count(data);
	
	return 0;

free3:
	hash_free(data->request_write);
free2:
	hash_free(data->request_read);
free:
	backend_destory(data->backend);
	return -ENOMEM;
}
/* }}} */

static ssize_t blocks_create(chain_t *chain, request_t *request, buffer_t *buffer){
	block_t           block;
	size_t            size;
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	if(hash_get_copy (request, "size",  TYPE_INT32,  &size, sizeof(size)) != 0)
		return -EINVAL;
	
	if(size > data->block_size) // TODO make sticked blocks
		return -EINVAL;
	
	if(map_get_block(data, data->blocks_count - 1, &block) != 0)
		// if blocks_count == 0, map_get_block will catch it
		return -1;
	
	if(size > block.free_size){
		//new_block()
	}
	// insert_to_block_end()
	return -1;
}

static ssize_t blocks_setgetdelete(chain_t *chain, request_t *request, buffer_t *buffer){
	// key = translate_key(key)
	// ret = chain_next_query(chain, request, buffer);
	return -1;
}

static ssize_t blocks_move(chain_t *chain, request_t *request, buffer_t *buffer){
	// key = translate_key(key)
	// size = (size > block_size) ? block_size : size;
	// ret = chain_next_query(chain, request, buffer);
	return -1;
}

static ssize_t blocks_count(chain_t *chain, request_t *request, buffer_t *buffer){
	// get_count()
	return -1;
}

static chain_t chain_blocks = {
	"blocks",
	CHAIN_TYPE_CRWD,
	&blocks_init,
	&blocks_configure,
	&blocks_destroy,
	{{
		.func_create = &blocks_create,
		.func_set    = &blocks_setgetdelete,
		.func_get    = &blocks_setgetdelete,
		.func_delete = &blocks_setgetdelete,
		.func_move   = &blocks_move,
		.func_count  = &blocks_count
	}}
};
CHAIN_REGISTER(chain_blocks)

