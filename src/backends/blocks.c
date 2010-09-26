#include <libfrozen.h>

typedef struct block_info {
	unsigned int free_size;
} block_info;

typedef struct blocks_user_data {
	size_t      block_size;
	size_t      blocks_count;
	backend_t  *backend;
	request_t  *request_read;
	request_t  *request_write;
	data_t     *request_read_key;
	data_t     *request_write_key;
	
} blocks_user_data;

// 'blocks' chain splits underlying data space into blocks.
// This is useful for reducing copy time in insert operations

static int blocks_init(chain_t *chain){
	blocks_user_data *user_data = calloc(1, sizeof(blocks_user_data));
	if(user_data == NULL)
		return -ENOMEM;
	
	chain->user_data = user_data;
	
	return 0;
}

static int blocks_destroy(chain_t *chain){
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	backend_destory(chain->user_data->backend);
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
	
	if( (data->backend = backend_new(config_backend)) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'backend' invalid");
	
	if( (data->request_read  = hash_new()) == NULL)
		goto free;
	
	if( (data->request_write = hash_new()) == NULL)
		goto free2;
	
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
	
	return 0;
	
free2:
	hash_free(data->request_read);
free:
	backend_destory(backend);
	return -ENOMEM;
}

static ssize_t blocks_create(chain_t *chain, request_t *request, buffer_t *buffer){
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
	}}
};
CHAIN_REGISTER(chain_blocks)

