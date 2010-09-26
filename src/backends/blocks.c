#include <libfrozen.h>

#define BLOCKS_DBG

typedef struct block_info {
	unsigned int  real_block_id; 
	unsigned int  free_size;
} block_info;

typedef struct block_t {
	off_t         real_block_id;
	size_t        free_size;
} block_t;

typedef struct blocks_user_data {
	size_t        block_size;
	off_t         blocks_count;
	backend_t    *backend;
	request_t    *request_create;
	request_t    *request_read;
	request_t    *request_write;
	request_t    *request_move;
	
	data_t       *request_read_key;
	data_t       *request_write_key;
	data_t       *request_write_data;
	buffer_t     *buffer_read;
	buffer_t     *buffer_create;
	
	request_t    *request_chain_layer;
} blocks_user_data;

// 'blocks' chain splits underlying data space into blocks.
// This is useful for reducing copy time in insert operations

/* private {{{ */
static void    map_get_blocks_count(blocks_user_data *data){ // {{{
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
	
	data->blocks_count = blocks_count / data->block_size;
	
	hash_free(request);
	buffer_free(buffer);
} // }}}
static int     map_new_block(chain_t *chain, request_t *request, off_t virt_block_id, block_t *p_block){ // {{{
	ssize_t           ret;
	off_t             block_id;
	off_t             block_id_to;
	block_info        info;
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	hash_empty        (data->request_chain_layer);
	hash_assign_layer (data->request_chain_layer, request);
	hash_set          (data->request_chain_layer, "size", TYPE_INT32, &data->block_size);
	
	ret = chain_next_query(chain, data->request_chain_layer, data->buffer_create);
	if(ret <= 0)
		return -EINVAL;
	
	buffer_read_flat(data->buffer_create, ret, &block_id, sizeof(block_id));
	block_id = block_id / data->block_size;
	
#ifdef BLOCKS_DBG
	printf("alloced %x real_block_id\n", (unsigned int)block_id);
#endif

	if(virt_block_id == data->blocks_count){
		// insert to map table
		ret = backend_query(data->backend, data->request_create, data->buffer_create);
		if(ret <= 0)
			return -EINVAL;
	}else{
		block_id_to = block_id + 1;
		
#ifdef BLOCKS_DBG
		printf("request_move from %x to %x\n", (unsigned int)block_id, (unsigned int)block_id_to);
#endif
		hash_set(data->request_move, "key_from", TYPE_INT64, &block_id);
		hash_set(data->request_move, "key_to",   TYPE_INT64, &block_id_to);
		
		ret = backend_query(data->backend, data->request_move, NULL);
		if(ret != 0)
			return -EINVAL;
	}
#ifdef BLOCKS_DBG
	printf("ok\n");
#endif
	
	info.real_block_id = block_id;
	info.free_size     = data->block_size;
	
	*(off_t *)(data->request_write_key) = virt_block_id;
	memcpy(data->request_write_data + 4, &info, sizeof(info));
	
	ret = backend_query(data->backend, data->request_write, NULL); 
	if(ret != 1)
		return -EINVAL;
	
	data->blocks_count++;
	
	p_block->free_size     = data->block_size;
	p_block->real_block_id = block_id;
	
	return 0;
} // }}}
static int     map_get_block(blocks_user_data *data, off_t block_id, block_t *block){ // {{{
	ssize_t       ret;
	block_info    info;
	
	if(block_id >= data->blocks_count || block == NULL)
		return -EINVAL;
	
	*(off_t *)(data->request_read_key) = block_id;
	ret = backend_query(data->backend, data->request_read, data->buffer_read);
	if(ret != 1) // one item
		return -EINVAL;
	
	buffer_read_flat(data->buffer_read, sizeof(info), &info, sizeof(info));
	
	if(info.free_size > data->block_size || info.real_block_id >= data->blocks_count)
		return -EINVAL;
	
	block->free_size     = info.free_size;
	block->real_block_id = info.real_block_id;
	
	return 0;
} // }}}
static int     map_set_block(blocks_user_data *data, off_t block_id, block_t *block){ // {{{
	ssize_t       ret;
	block_info    info;
	
	if(block_id >= data->blocks_count || block == NULL)
		return -EINVAL;
	
	info.real_block_id = block->real_block_id;
	info.free_size     = block->free_size;
	
	*(off_t *)(data->request_write_key) = block_id;
	memcpy(data->request_write_data + 4, &info, sizeof(info));
	
	ret = backend_query(data->backend, data->request_write, NULL);
	if(ret != 1)
		return -EINVAL;
	
	return 0;
} // }}}

// TODO remove TYPE_INT64 from code, support all

static int    map_translate_key(blocks_user_data *data, char *key_name, request_t *request){ //
	off_t        key;
	off_t        virt_block_id;
	block_t      block;
	
	if(hash_get_copy (request, key_name, TYPE_INT64, &key, sizeof(key)) != 0)
		return -EINVAL;
	
	virt_block_id = key / data->block_size;
	
	if(map_get_block(data, virt_block_id, &block) != 0)
		return -EINVAL;
	
	//printf("key was: %x br: %x bv: %x\n", (unsigned int)key, (unsigned int)block.real_block_id, (unsigned int));
	key = key + (block.real_block_id - virt_block_id) * data->block_size;
	
	hash_set(data->request_chain_layer, key_name, TYPE_INT64, &key);
	
	return 0;
}

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
	char              value_data[] = "\x0c\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00";
	blocks_user_data *data         = (blocks_user_data *)chain->user_data;
	
	if( (block_size_str = setting_get_child_string(config, "block_size")) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'block_size' not set\n");
	
	if( (data->block_size = strtoul(block_size_str, NULL, 10)) == 0)
		return_error(-EINVAL, "chain 'blocks' variable 'block_size' invalid\n");
	
	if( (config_backend = setting_get_child_setting(config, "backend")) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'backend' not set\n");
	
	if( (data->backend = backend_new("blocks_map", config_backend)) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'backend' invalid\n");
	
	if( (data->request_create = hash_new()) == NULL)
		goto free;
	
	if( (data->request_read  = hash_new()) == NULL)
		goto free1;
	
	if( (data->request_write = hash_new()) == NULL)
		goto free2;
	
	if( (data->request_move = hash_new()) == NULL)
		goto free3;
	
	if( (data->buffer_read = buffer_alloc()) == NULL)
		goto free4;
	
	if( (data->buffer_create = buffer_alloc()) == NULL)
		goto free5;
	
	if( (data->request_chain_layer = hash_new()) == NULL)
		goto free6;
	
	action = ACTION_CRWD_CREATE;
	hash_set(data->request_create, "action", TYPE_INT32, &action);
	action = ACTION_CRWD_READ;	
	hash_set(data->request_read,   "action", TYPE_INT32, &action);
	action = ACTION_CRWD_WRITE;
	hash_set(data->request_write,  "action", TYPE_INT32, &action);
	action = ACTION_CRWD_MOVE;
	hash_set(data->request_move,   "action", TYPE_INT32, &action);
	
	key    = 0;
	hash_set(data->request_read,  "key",    TYPE_INT64,  &key);
	hash_set(data->request_write, "key",    TYPE_INT64,  &key);
	hash_set(data->request_write, "value",  TYPE_BINARY, &value_data);
	hash_get_any(data->request_read,  "key",   NULL, &data->request_read_key,   NULL);
	hash_get_any(data->request_write, "key",   NULL, &data->request_write_key,  NULL);
	hash_get_any(data->request_write, "value", NULL, &data->request_write_data, NULL);
	
	size   = 1;
	hash_set(data->request_create, "size",   TYPE_INT32, &size);
	hash_set(data->request_read,   "size",   TYPE_INT32, &size);
	hash_set(data->request_write,  "size",   TYPE_INT32, &size);
	
	map_get_blocks_count(data);
	
	return 0;
	
free6:  buffer_free(data->buffer_create);
free5:  buffer_free(data->buffer_read);
free4:  hash_free(data->request_move);
free3:	hash_free(data->request_write);
free2:	hash_free(data->request_read);
free1:  hash_free(data->request_create);
free:	backend_destory(data->backend);
	return -ENOMEM;
}
/* }}} */

static ssize_t blocks_create(chain_t *chain, request_t *request, buffer_t *buffer){
	block_t           block;
	off_t             block_id;
	unsigned int      element_size;
	off_t             virt_ptr;
	
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	if(hash_get_copy (request, "size",  TYPE_INT32,  &element_size, sizeof(element_size)) != 0)
		return -EINVAL;
	
	if(element_size > data->block_size) // TODO make sticked blocks
		return -EINVAL;
	
	/* if files empty - create new block */
	if(data->blocks_count == 0){
		if(map_new_block(chain, request, 0, &block) != 0)
			return -EFAULT;
	}
	
	/* try write into last block */
	block_id = data->blocks_count - 1;
	if(map_get_block(data, block_id, &block) != 0)
		return -EFAULT;
	
	/* if not enough space - create new one */
	if(element_size > block.free_size){
		block_id = data->blocks_count;
		if(map_new_block(chain, request, block_id, &block) != 0)
			return -EFAULT;
	}
	
	/* calc virt_ptr */
	virt_ptr = block_id * data->block_size + (data->block_size - block.free_size);
	
	/* save block params */
	block.free_size -= element_size;
	if(map_set_block(data, block_id, &block) != 0)
		return -EFAULT;
	
	buffer_write(buffer, sizeof(off_t), *(off_t *)chunk = virt_ptr); 
	return sizeof(off_t);
}

static ssize_t blocks_setgetdelete(chain_t *chain, request_t *request, buffer_t *buffer){
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	hash_empty        (data->request_chain_layer);
	hash_assign_layer (data->request_chain_layer, request);
	
	if(map_translate_key(data, "key", request) != 0)
		return -EINVAL;
	
	return chain_next_query(chain, data->request_chain_layer, buffer);
}

static ssize_t blocks_move(chain_t *chain, request_t *request, buffer_t *buffer){
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	hash_empty        (data->request_chain_layer);
	hash_assign_layer (data->request_chain_layer, request);
	
	//if(map_translate_key(data, "key_from", request) != 0)
	//	return -EINVAL;
	//if(map_translate_key(data, "key_to",   request) != 0)
	//	return -EINVAL;
	
	
	return chain_next_query(chain, data->request_chain_layer, buffer);
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

