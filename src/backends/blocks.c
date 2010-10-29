#include <libfrozen.h>

#define BLOCKS_DBG

typedef struct block_info {
	unsigned int  real_block_off; 
	unsigned int  size;
} block_info;

typedef struct blocks_user_data {
	backend_t    *bk_map;
	chain_t      *ch_real;
	
	size_t        block_size;
	off_t         blocks_count;
	
} blocks_user_data;

// 'blocks' chain splits underlying data space into blocks.
// This is useful for reducing copy time in insert operations

static ssize_t   map_new            (blocks_user_data *data, unsigned int b_off, unsigned int b_size){ // {{{
	hash_t  req_write[] = {
		{ "action",     DATA_INT32(ACTION_CRWD_WRITE)  },
		{ "block_off",  DATA_PTR_INT32(&b_off)         },
		{ "block_size", DATA_PTR_INT32(&b_size)        },
		hash_end
	};
	return backend_query(data->bk_map, req_write, NULL);
} // }}}
static ssize_t   map_insert         (blocks_user_data *data, unsigned int b_vid, unsigned int b_off, unsigned int b_size){ // {{{
	hash_t  req_write[] = {
		{ "action",     DATA_INT32(ACTION_CRWD_WRITE)  },
		{ "block_vid",  DATA_PTR_INT32(&b_vid)         },
		{ "block_off",  DATA_PTR_INT32(&b_off)         },
		{ "block_size", DATA_PTR_INT32(&b_size)        },
		hash_end
	};
	return backend_query(data->bk_map, req_write, NULL);
} // }}}
static ssize_t   map_resize         (blocks_user_data *data, unsigned int b_vid, unsigned int new_size){ // {{{
	hash_t  req_write[] = {
		{ "action",     DATA_INT32(ACTION_CRWD_WRITE)  },
		{ "block_vid",  DATA_PTR_INT32(&b_vid)         },
		{ "block_size", DATA_PTR_INT32(&new_size)      },
		hash_end
	};
	return backend_query(data->bk_map, req_write, NULL);
} // }}}
/*static ssize_t   map_delete         (blocks_user_data *data, unsigned int b_vid){ // {{{
	hash_t  req_delete[] = {
		{ "action",     DATA_INT32(ACTION_CRWD_DELETE) },
		{ "block_vid",  DATA_PTR_INT32(&b_vid)         },
		hash_end
	};
	return backend_query(data->bk_map, req_delete, NULL);
} // }}}*/
static ssize_t   map_off            (blocks_user_data *data, off_t virt_off, unsigned int *block_vid, off_t *real_off){ // {{{
	ssize_t  ret;
	ssize_t  buf_ptr = 0;
	
	buffer_t  req_buffer;
	buffer_init(&req_buffer);
	
	hash_t    req_read[] = {
		{ "action", DATA_INT32(ACTION_CRWD_READ) },
		{ "offset", DATA_PTR_OFFT(&virt_off)     },
		hash_end
	};
	
	ret = backend_query(data->bk_map, req_read, &req_buffer);
	if(ret > 0){
		buf_ptr += buffer_read(&req_buffer, buf_ptr, real_off,  sizeof(off_t));
		buf_ptr += buffer_read(&req_buffer, buf_ptr, block_vid, sizeof(unsigned int));
	}
	buffer_destroy(&req_buffer);
	return ret;
} // }}}
static ssize_t   map_get_block      (blocks_user_data *data, unsigned int block_vid, off_t *real_off, size_t *block_size){ // {{{
	ssize_t  ret;
	ssize_t  buf_ptr = 0;
	
	buffer_t  req_buffer;
	buffer_init(&req_buffer);
	
	hash_t    req_read[] = {
		{ "action",     DATA_INT32(ACTION_CRWD_READ) },
		{ "blocks",     DATA_INT32(1)                },
		{ "block_vid",  DATA_PTR_INT32(&block_vid)   },
		hash_end
	};
	
	ret = backend_query(data->bk_map, req_read, &req_buffer);
	if(ret > 0){
		*real_off   = 0;
		*block_size = 0;
		
		buf_ptr += buffer_read(&req_buffer, buf_ptr, block_size, MIN(ret, sizeof(unsigned int))); // TODO bad
		buf_ptr += buffer_read(&req_buffer, buf_ptr, real_off,   MIN(ret, sizeof(unsigned int)));
	}
	buffer_destroy(&req_buffer);
	return ret;
} // }}}
static ssize_t   map_blocks         (blocks_user_data *data){ // {{{
	ssize_t  ret;
	
	buffer_t req_buffer;
	buffer_init_from_bare(&req_buffer, &data->blocks_count, sizeof(data->blocks_count));
	
	data->blocks_count = 0;
	
	hash_t  req_count[] = {
		{ "action", DATA_INT32(ACTION_CRWD_COUNT) },
		{ "blocks", DATA_INT32(1)                 },
		hash_end
	};
	ret = backend_query(data->bk_map, req_count, &req_buffer);
	buffer_destroy(&req_buffer);
	return ret;
} // }}}
static ssize_t   real_new           (blocks_user_data *data, off_t *real_block_off){ // {{{
	ssize_t   ret;
	
	buffer_t  req_buffer;
	buffer_init_from_bare(&req_buffer, real_block_off, sizeof(off_t));
	
	hash_t    req_create[] = {
		{ "action", DATA_INT32(ACTION_CRWD_CREATE ) },
		{ "size"  , DATA_INT32(data->block_size   ) },
		hash_end
	};
	
	ret = chain_next_query(data->ch_real, req_create, &req_buffer);
	buffer_destroy(&req_buffer);
	return ret;
} // }}}
static ssize_t   real_move          (blocks_user_data *data, off_t from, off_t to, size_t size){ // {{{
	if(from == to || size == 0)
		return 0;
	
	hash_t    req_move[] = {
		{ "action",   DATA_INT32(ACTION_CRWD_MOVE) },
		{ "key_from", DATA_PTR_OFFT(&from)         },
		{ "key_to",   DATA_PTR_OFFT(&to)           },
		{ "size"  ,   DATA_INT32(size)             },
		hash_end
	};
	
	return chain_next_query(data->ch_real, req_move, NULL);
} // }}}
static ssize_t   itms_insert_copy   (chain_t *chain, off_t src, off_t dst, size_t size){ // {{{
	off_t         src_real,      dst_real;
	unsigned int  src_block_vid, dst_block_vid;
	unsigned int  dst_block_off;
	unsigned int  new_block_vid;
	off_t         src_block_roff;
	off_t         new_block_roff;
	
	//off_t         block_roff;
	size_t        block_size;
	
	// TODO copy only data from src blocks instead of whole block
	// TODO eh.. how it works?
	// NOTE this is HELL!
	
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	if(map_off(data, src, &src_block_vid, &src_real) <= 0)
		return -EINVAL;
	if(map_off(data, dst, &dst_block_vid, &dst_real) <= 0)
		return -EINVAL;
	
	dst_block_off = dst_real % data->block_size;
	
	/*
	if(src_block_vid == dst_block_vid){
		if(map_get_block(data, src_block_vid, &block_roff, &block_size) <= 0)
			return -EFAULT;
		
		if(block_size + size <= data->block_size){
			if(real_move(data, src_real, dst_real + size, (block_size - dst_block_off)) != 0)
				return -EFAULT;
			if(map_resize(data, src_block_vid, block_size + size) != 0)
				return -EFAULT;
			return 0;
		}
	}*/
	
	if(dst_block_off != 0){ // inserting data in middle, we truncate block and insert data after it
		block_size = data->block_size - dst_block_off;
		
		if(real_new(data, &new_block_roff) <= 0)
			return -EFAULT;
		
		if(real_move(data, dst_real, new_block_roff, block_size) != 0)
			return -EFAULT;
		
		if(map_insert(data, dst_block_vid + 1, (unsigned int)new_block_roff, block_size) != 0)
			return -EFAULT;
		
		if(map_resize(data, dst_block_vid, dst_block_off) != 0)
			return -EFAULT;
	}
	
	if(map_off(data, src, &src_block_vid, &src_real) <= 0)
		return -EINVAL;
	
	new_block_vid = src_block_vid;
	
	goto start;
	while(size > 0){
		if(map_off(data, src, &src_block_vid, &src_real) <= 0)
			return -EINVAL;
		
	start:
		if(map_get_block(data, src_block_vid, &src_block_roff, &block_size) <= 0)
			return -EFAULT;
		
		block_size = MIN(size, block_size);
		
		if(real_new(data, &new_block_roff) <= 0)
			return -EFAULT;
		
		if(real_move(data, src_real, new_block_roff, block_size) != 0)
			return -EFAULT;
		
		if(map_insert(data, new_block_vid, (unsigned int)new_block_roff, block_size) != 0)
			return -EFAULT;
		
		src        += block_size * 2; // TODO oh my...
		size       -= block_size;
		new_block_vid++;
	}
	
	return 0;
} // }}}
static ssize_t   itms_delete        (chain_t *chain, off_t start, size_t size){ // {{{
	off_t         start_real,      end_real;
	unsigned int  start_block_off, end_block_off;
	unsigned int  start_block_vid, end_block_vid, i_vid;
	unsigned int  end_block_aoff;                          // absolute offset
	off_t         block_roff;
	size_t        block_size;
	
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	if(map_off(data, start,        &start_block_vid, &start_real) <= 0)
		return -EFAULT;
	if(map_off(data, start + size, &end_block_vid,   &end_real)   <= 0){
		end_block_vid = (data->blocks_count - 1);
		end_block_off = data->block_size;
	}else{
		end_block_off   = end_real   % data->block_size;
		end_block_aoff  = (end_real - end_block_off);
	}
	start_block_off = start_real % data->block_size;
	
	for(i_vid = start_block_vid; i_vid <= end_block_vid; i_vid++){
		if(i_vid == end_block_vid){
			if(map_get_block(data, end_block_vid, &block_roff, &block_size) <= 0)
				return -EFAULT;
			
			if(real_move(data, end_real, block_roff, block_size - end_block_off) != 0) // TODO WRRRRRROOOOOOOOOOOONGG
				return -EFAULT;
				
			if(map_resize(data, end_block_vid, block_size - end_block_off) != 0)
				return -EFAULT;
			
			break;
		}
		if(i_vid == start_block_vid){ // first block
			if(map_resize(data, start_block_vid, start_block_off) != 0)
				return -EFAULT;
			
			continue;
		}
		
		if(map_resize(data, i_vid, 0) != 0)
			return -EFAULT;
	}
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
	
	backend_destroy(data->bk_map);
	free(chain->user_data);
	
	chain->user_data = NULL;
	return 0;
}

static int blocks_configure(chain_t *chain, hash_t *config){
	void             *temp;
	hash_t           *r_backend;
	blocks_user_data *data         = (blocks_user_data *)chain->user_data;
	
	data->block_size =
		(hash_get_typed(config, TYPE_INT32, "block_size", &temp, NULL) == 0) ?
		*(unsigned int *)temp : 0;
	
	if( data->block_size == 0)
		return_error(-EINVAL, "chain 'blocks' variable 'block_size' invalid\n");
	
	if( (r_backend = hash_find(config, "backend")) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'backend' not set\n");
	
	if( (data->bk_map = backend_new(r_backend)) == NULL)
		return_error(-EINVAL, "chain 'blocks' variable 'backend' invalid\n");
	
	data->ch_real = chain;
	
	map_blocks(data);
	return 0;
}
/* }}} */

static ssize_t blocks_create(chain_t *chain, request_t *request, buffer_t *buffer){ // {{{
	unsigned int      element_size;
	
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	off_t             block_off;
	size_t            block_size;
	off_t             block_vid;
	ssize_t           ret;
	hash_t           *r_size;
	
	if( (r_size = hash_find_typed (request, TYPE_INT32, "size")) == NULL)
		return -EINVAL;
	element_size = HVALUE(r_size, unsigned int);
	
	if(element_size > data->block_size) // TODO make sticked blocks
		return -EINVAL;
	
	/* if file empty - create new block */
	if(data->blocks_count == 0){
		if(real_new(data, &block_off) <= 0)
			return -EFAULT;
		
		if(map_new(data, block_off, 0) != 0)
			return -EFAULT;
		
		if(map_blocks(data) <= 0)
			return -EFAULT;
	}
	
	/* try write into last block */
	block_vid = data->blocks_count - 1;
	if(map_get_block(data, block_vid, &block_off, &block_size) <= 0)
		return -EFAULT;
	
	/* if not enough space - create new one */
	if(element_size + block_size > data->block_size){
		if(real_new(data, &block_off) <= 0) // TODO get block_vid too
			return -EFAULT;
		
		if(map_new(data, block_off, 0) != 0)
			return -EFAULT;
		
		if(map_blocks(data) <= 0)
			return -EFAULT;
		
		block_vid  = data->blocks_count - 1;
		block_size = 0;
	}
	
	/* calc virt_ptr */
	hash_t  req_count[] = {
		{ "action", DATA_INT32(ACTION_CRWD_COUNT) },
		hash_end
	};
	ret = backend_query(data->bk_map, req_count, buffer); 
	
	/* really add item */
	if(map_resize(data, block_vid, block_size + element_size) != 0)
		return -EFAULT;
	
	return ret;
} // }}}
static ssize_t blocks_setget(chain_t *chain, request_t *request, buffer_t *buffer){ // {{{
	off_t             key_real;
	unsigned int      block_vid;
	hash_t           *r_key_virt;
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	// TODO r_size > block_size
	// TODO remove TYPE_OFFT from code, support all
	
	if( (r_key_virt = hash_find_typed(request, TYPE_OFFT, "key")) == NULL)
		return -EINVAL;
	
	if(map_off(data, HVALUE(r_key_virt, off_t), &block_vid, &key_real) <= 0)
		return -EINVAL;
	
	hash_t  req_layer[] = {
		{ r_key_virt->key, r_key_virt->type, &key_real, r_key_virt->value_size },
		hash_next(request)
	};
	
	return chain_next_query(chain, req_layer, buffer);
} // }}}
static ssize_t blocks_delete(chain_t *chain, request_t *request, buffer_t *buffer){ // {{{
	hash_t *r_key, *r_size;
	
	if( (r_key  = hash_find_typed(request, TYPE_OFFT,  "key")) == NULL)
		return -EINVAL;
	if( (r_size = hash_find_typed(request, TYPE_INT32, "size")) == NULL)
		return -EINVAL;
	
	return itms_delete(chain, HVALUE(r_key, off_t), HVALUE(r_size, unsigned int));
} // }}}
static ssize_t blocks_move(chain_t *chain, request_t *request, buffer_t *buffer){ // {{{
	off_t             from, to;
	unsigned int      size;
	hash_t           *r_from, *r_to, *r_size;
	
	ssize_t           move_delta;
	
	if( (r_from = hash_find_typed (request, TYPE_OFFT, "key_from")) == NULL)
		return -EINVAL;
	if( (r_to   = hash_find_typed (request, TYPE_OFFT, "key_to"))   == NULL)
		return -EINVAL;
	
	from = HVALUE(r_from, off_t);
	to   = HVALUE(r_to,   off_t);
	
	if(from == to)
		return 0;
	
	// manage top of move
	move_delta = (to - from);
	if(move_delta > 0){ // moving data forward
		if(itms_insert_copy(chain, from, from, move_delta) != 0)
			return -EINVAL;
	}else{ // moving data backward
		if(itms_delete(chain, to, -move_delta) != 0)
			return -EINVAL;
	}
	
	// manage bottom of move
	if( (r_size = hash_find_typed (request, TYPE_INT32, "size")) == NULL)
		return 0;
	size = HVALUE(r_size, unsigned int);
	if(size == -1)
		return 0;
		
	from += size;
	to   += size;
	
	if(move_delta > 0){ // moving data forward
		if(itms_delete(chain, from, move_delta) != 0)
			return -EINVAL;
	}else{ // moving data backward
		if(itms_insert_copy(chain, to, from, -move_delta) != 0)
			return -EINVAL;
	}
	return 0;
} // }}}
static ssize_t blocks_count(chain_t *chain, request_t *request, buffer_t *buffer){ // {{{
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	return backend_query(data->bk_map, request, buffer);  // transfer query to map backend _COUNT
} // }}}

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

/*
static ssize_t   itms_insert_copy   (chain_t *chain, off_t src, off_t dst, size_t size){ // {{{
	off_t         src_real,      dst_real;
	unsigned int  src_block_vid, dst_block_vid;
	unsigned int  dst_block_off;
	unsigned int  new_block_vid;
	off_t         src_block_roff;
	off_t         new_block_roff;
	
	//off_t         block_roff;
	size_t        block_size;
	
	// TODO copy only data from src blocks instead of whole block
	// TODO eh.. how it works?
	// NOTE this is HELL!
	
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	if(map_off(data, src, &src_block_vid, &src_real) <= 0)
		return -EINVAL;
	if(map_off(data, dst, &dst_block_vid, &dst_real) <= 0)
		return -EINVAL;
	
	dst_block_off = dst_real % data->block_size;
	
	if(dst_block_off != 0){ // inserting data in middle, we truncate block and insert data after it
		block_size = data->block_size - dst_block_off;
		
		if(real_new(data, &new_block_roff) <= 0)
			return -EFAULT;
		
		if(real_move(data, dst_real, new_block_roff, block_size) != 0)
			return -EFAULT;
		
		if(map_insert(data, dst_block_vid + 1, (unsigned int)new_block_roff, block_size) != 0)
			return -EFAULT;
		
		if(map_resize(data, dst_block_vid, dst_block_off) != 0)
			return -EFAULT;
	}
	
	if(map_off(data, src, &src_block_vid, &src_real) <= 0)
		return -EINVAL;
	
	new_block_vid = src_block_vid;
	
	goto start;
	while(size > 0){
		if(map_off(data, src, &src_block_vid, &src_real) <= 0)
			return -EINVAL;
		
	start:
		if(map_get_block(data, src_block_vid, &src_block_roff, &block_size) <= 0)
			return -EFAULT;
		
		block_size = MIN(size, block_size);
		
		if(real_new(data, &new_block_roff) <= 0)
			return -EFAULT;
		
		if(real_move(data, src_real, new_block_roff, block_size) != 0)
			return -EFAULT;
		
		if(map_insert(data, new_block_vid, (unsigned int)new_block_roff, block_size) != 0)
			return -EFAULT;
		
		src        += block_size * 2; // TODO oh my...
		size       -= block_size;
		new_block_vid++;
	}
	
	return 0;
} // }}}
static ssize_t   itms_delete        (chain_t *chain, off_t start, size_t size){ // {{{
	off_t         start_real,      end_real;
	unsigned int  start_block_off, end_block_off;
	unsigned int  start_block_vid, end_block_vid, i_vid;
	unsigned int  end_block_aoff;                          // absolute offset
	off_t         block_roff;
	size_t        block_size;
	
	blocks_user_data *data = (blocks_user_data *)chain->user_data;
	
	if(map_off(data, start,        &start_block_vid, &start_real) <= 0)
		return -EFAULT;
	if(map_off(data, start + size, &end_block_vid,   &end_real)   <= 0){
		end_block_vid = (data->blocks_count - 1);
		end_block_off = data->block_size;
	}else{
		end_block_off   = end_real   % data->block_size;
		end_block_aoff  = (end_real - end_block_off);
	}
	start_block_off = start_real % data->block_size;
	
	for(i_vid = start_block_vid; i_vid <= end_block_vid; i_vid++){
		if(i_vid == end_block_vid){
			if(map_get_block(data, end_block_vid, &block_roff, &block_size) <= 0)
				return -EFAULT;
			
			if(real_move(data, end_real, block_roff, block_size - end_block_off) != 0) // TODO WRRRRRROOOOOOOOOOOONGG
				return -EFAULT;
				
			if(map_resize(data, end_block_vid, block_size - end_block_off) != 0)
				return -EFAULT;
			
			break;
		}
		if(i_vid == start_block_vid){ // first block
			if(map_resize(data, start_block_vid, start_block_off) != 0)
				return -EFAULT;
			
			continue;
		}
		
		if(map_resize(data, i_vid, 0) != 0)
			return -EFAULT;
	}
	return 0;
} // }}}
*/
