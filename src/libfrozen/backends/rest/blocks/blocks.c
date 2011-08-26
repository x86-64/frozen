#include <libfrozen.h>

#define BLOCKS_DBG

typedef struct block_info {
	unsigned int  real_block_off; 
	unsigned int  size;
} block_info;

typedef struct blocks_userdata {
	backend_t    *bk_map;
	backend_t      *ch_real;
	
	size_t        block_size;
	off_t         blocks_count;
	
} blocks_userdata;

// 'blocks' chain splits underlying data space into blocks.
// This is useful for reducing copy time in insert operations

static ssize_t   map_new            (blocks_userdata *data, unsigned int b_off, unsigned int b_size){ // {{{
	hash_t  req_write[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_WRITE)  },
		{ HK(block_off),  DATA_PTR_UINT32T(&b_off)         },
		{ HK(block_size), DATA_PTR_UINT32T(&b_size)        },
		hash_end
	};
	return backend_query(data->bk_map, req_write);
} // }}}
static ssize_t   map_insert         (blocks_userdata *data, unsigned int b_vid, unsigned int b_off, unsigned int b_size){ // {{{
	hash_t  req_write[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_WRITE)  },
		{ HK(block_vid),  DATA_PTR_UINT32T(&b_vid)         },
		{ HK(block_off),  DATA_PTR_UINT32T(&b_off)         },
		{ HK(block_size), DATA_PTR_UINT32T(&b_size)        },
		hash_end
	};
	return backend_query(data->bk_map, req_write);
} // }}}
static ssize_t   map_resize         (blocks_userdata *data, unsigned int b_vid, unsigned int new_size){ // {{{
	hash_t  req_write[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_WRITE)  },
		{ HK(block_vid),  DATA_PTR_UINT32T(&b_vid)         },
		{ HK(block_size), DATA_PTR_UINT32T(&new_size)      },
		hash_end
	};
	return backend_query(data->bk_map, req_write);
} // }}}
/*static ssize_t   map_delete         (blocks_userdata *data, unsigned int b_vid){ // {{{
	hash_t  req_delete[] = {
		{ HK(action),     DATA_UINT32T(ACTION_CRWD_DELETE) },
		{ HK(block_vid),  DATA_PTR_UINT32T(&b_vid)         },
		hash_end
	};
	return backend_query(data->bk_map, req_delete);
} // }}}*/
static ssize_t   map_off            (blocks_userdata *data, off_t virt_off, unsigned int *block_vid, off_t *real_off){ // {{{
	hash_t    req_read[] = {
		{ HK(action),       DATA_UINT32T(ACTION_CRWD_READ) },
		{ HK(offset),       DATA_PTR_OFFT(&virt_off)     },
		{ HK(real_offset),  DATA_PTR_OFFT(real_off)      },
		{ HK(block_vid),    DATA_PTR_UINT32T(block_vid)    },
		hash_end
	};
	
	return backend_query(data->bk_map, req_read);
} // }}}
static ssize_t   map_get_block      (blocks_userdata *data, unsigned int block_vid, off_t *real_off, size_t *block_size){ // {{{
	hash_t    req_read[] = {
		{ HK(action),      DATA_UINT32T(ACTION_CRWD_READ) },
		{ HK(blocks),      DATA_UINT32T(1)                },
		{ HK(block_vid),   DATA_PTR_UINT32T(&block_vid)   },
		{ HK(block_size),  DATA_PTR_UINT32T(block_size)   },
		{ HK(real_offset), DATA_PTR_OFFT(real_off)      },
		hash_end
	};
	
	return backend_query(data->bk_map, req_read);
} // }}}
static ssize_t   map_blocks         (blocks_userdata *data){ // {{{
	ssize_t  ret;
	
	buffer_t req_buffer;
	buffer_init_from_bare(&req_buffer, &data->blocks_count, sizeof(data->blocks_count));
	
	data->blocks_count = 0;
	
	hash_t  req_count[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_COUNT) },
		{ HK(blocks), DATA_UINT32T(1)                 },
		{ HK(buffer), DATA_BUFFERT(&req_buffer)     },
		hash_end
	};
	ret = backend_query(data->bk_map, req_count);
	buffer_destroy(&req_buffer);
	return ret;
} // }}}
static ssize_t   real_new           (blocks_userdata *data, off_t *real_block_off){ // {{{
	ssize_t   ret;
	
	buffer_t  req_buffer;
	buffer_init_from_bare(&req_buffer, real_block_off, sizeof(off_t));
	
	hash_t    req_create[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_CREATE ) },
		{ HK(size)  , DATA_SIZET(data->block_size   ) },
		{ HK(buffer), DATA_BUFFERT(&req_buffer)       },
		hash_end
	};
	
	ret = chain_next_query(data->ch_real, req_create);
	buffer_destroy(&req_buffer);
	return ret;
} // }}}
static ssize_t   real_move          (blocks_userdata *data, off_t from, off_t to, size_t size){ // {{{
	if(from == to || size == 0)
		return 0;
	
	hash_t    req_move[] = {
		{ HK(action),   DATA_UINT32T(ACTION_CRWD_MOVE) },
		{ HK(offset_from), DATA_PTR_OFFT(&from)         },
		{ HK(offset_to),   DATA_PTR_OFFT(&to)           },
		{ HK(size)  ,   DATA_SIZET(size)             },
		hash_end
	};
	
	return chain_next_query(data->ch_real, req_move);
} // }}}
static ssize_t   itms_insert_copy   (backend_t *chain, off_t src, off_t dst, size_t size){ // {{{
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
	
	blocks_userdata *data = (blocks_userdata *)chain->userdata;
	
	if(map_off(data, src, &src_block_vid, &src_real) != 0)
		return -EINVAL;
	if(map_off(data, dst, &dst_block_vid, &dst_real) != 0)
		return -EINVAL;
	
	dst_block_off = dst_real % data->block_size;
	
	/*
	if(src_block_vid == dst_block_vid){
		if(map_get_block(data, src_block_vid, &block_roff, &block_size) != 0)
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
	
	if(map_off(data, src, &src_block_vid, &src_real) != 0)
		return -EINVAL;
	
	new_block_vid = src_block_vid;
	
	goto start;
	while(size > 0){
		if(map_off(data, src, &src_block_vid, &src_real) != 0)
			return -EINVAL;
		
	start:
		if(map_get_block(data, src_block_vid, &src_block_roff, &block_size) != 0)
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
static ssize_t   itms_delete        (backend_t *chain, off_t start, size_t size){ // {{{
	off_t         start_real,      end_real;
	unsigned int  start_block_off, end_block_off;
	unsigned int  start_block_vid, end_block_vid, i_vid;
	unsigned int  end_block_aoff;                          // absolute offset
	off_t         block_roff;
	size_t        block_size;
	
	blocks_userdata *data = (blocks_userdata *)chain->userdata;
	
	if(map_off(data, start,        &start_block_vid, &start_real) != 0)
		return -EFAULT;
	if(map_off(data, start + size, &end_block_vid,   &end_real)   != 0){
		end_block_vid = (data->blocks_count - 1);
		end_block_off = data->block_size;
	}else{
		end_block_off   = end_real   % data->block_size;
		end_block_aoff  = (end_real - end_block_off);
	}
	start_block_off = start_real % data->block_size;
	
	for(i_vid = start_block_vid; i_vid <= end_block_vid; i_vid++){
		if(i_vid == end_block_vid){
			if(map_get_block(data, end_block_vid, &block_roff, &block_size) != 0)
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

/* init {{ */
static int blocks_init(backend_t *chain){
	blocks_userdata *userdata = calloc(1, sizeof(blocks_userdata));
	if(userdata == NULL)
		return -ENOMEM;
	
	chain->userdata = userdata;
	
	return 0;
}

static int blocks_destroy(backend_t *chain){
	blocks_userdata *data = (blocks_userdata *)chain->userdata;
	
	backend_destroy(data->bk_map);
	free(chain->userdata);
	
	chain->userdata = NULL;
	return 0;
}

static int blocks_configure(backend_t *chain, hash_t *config){
	ssize_t           ret;
	hash_t           *r_backend;
	blocks_userdata  *data         = (blocks_userdata *)chain->userdata;
	
	hash_data_copy(ret, TYPE_UINT32T, data->block_size, config, HK(block_size));
	if(ret != 0)
		return error("chain blocks variable 'block_size' invalid");
	
	if( (r_backend = hash_find(config, HK(backend))) == NULL)
		return error("chain blocks variable 'backend' not set");
	
	if( (data->bk_map = backend_new(r_backend)) == NULL)
		return error("chain blocks variable 'backend' invalid");
	
	data->ch_real = chain;
	
	map_blocks(data);
	return 0;
}
/* }} */

static ssize_t blocks_create(backend_t *chain, request_t *request){ // {{{
	unsigned int      element_size;
	
	blocks_userdata *data = (blocks_userdata *)chain->userdata;
	off_t             block_off;
	size_t            block_size;
	off_t             block_vid;
	ssize_t           ret;
	
	hash_data_copy(ret, TYPE_SIZET, element_size, request, HK(size));
	if(ret != 0)
		return -EINVAL;

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
	if(map_get_block(data, block_vid, &block_off, &block_size) != 0)
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
		{ HK(action), DATA_UINT32T(ACTION_CRWD_COUNT) },
		hash_next(request)
	};
	ret = backend_query(data->bk_map, req_count); 
	
	/* really add item */
	if(map_resize(data, block_vid, block_size + element_size) != 0)
		return -EFAULT;
	
	return ret;
} // }}}
static ssize_t blocks_setget(backend_t *chain, request_t *request){ // {{{
	unsigned int      block_vid;
	hash_t           *r_key_virt;
	off_t             key_real, key_virt;
	data_t            key_real_dt = DATA_PTR_OFFT(&key_real);
	blocks_userdata *data = (blocks_userdata *)chain->userdata;
	
	// TODO r_size > block_size
	// TODO remove TYPE_OFFT from code, support all
	
	hash_data_copy(ret, TYPE_OFFT, key_virt, request, HK(offset));
	if(ret != 0)
		return -EINVAL;
	
	if(map_off(data, key_virt, &block_vid, &key_real) != 0)
		return -EINVAL;
	
	hash_t  req_layer[] = {
		{ HK(offset), key_real_dt           },
		hash_next(request)
	};
	
	return chain_next_query(chain, req_layer);
} // }}}
static ssize_t blocks_delete(backend_t *chain, request_t *request){ // {{{
	ssize_t ret;
	off_t   r_key;
	size_t  r_size;
	
	hash_data_copy(ret, TYPE_OFFT,  r_key,  request, HK(offset)); if(ret != 0) return warning("no offset supplied");
	hash_data_copy(ret, TYPE_SIZET, r_size, request, HK(size)); if(ret != 0) return warning("no size supplied");
	
	return itms_delete(chain, r_key, r_size);
} // }}}
static ssize_t blocks_move(backend_t *chain, request_t *request){ // {{{
	ssize_t           ret;
	off_t             from, to;
	unsigned int      size;
	
	ssize_t           move_delta;
	
	hash_data_copy(ret, TYPE_OFFT,  from,  request, HK(offset_from)); if(ret != 0) return warning("no offset_from supplied");
	hash_data_copy(ret, TYPE_OFFT,  to,    request, HK(offset_to)); if(ret != 0) return warning("no offset_to supplied");
	
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
	hash_data_copy(ret, TYPE_SIZET, size, request, HK(size));
	if(ret != 0 || size == -1)
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
static ssize_t blocks_count(backend_t *chain, request_t *request){ // {{{
	blocks_userdata *data = (blocks_userdata *)chain->userdata;
	
	return backend_query(data->bk_map, request);  // transfer query to map backend _COUNT
} // }}}

backend_t blocks_proto = {
	.class          = "blocks",
	.supported_api  = API_CRWD,
	.func_init      = &blocks_init,
	.func_configure = &blocks_configure,
	.func_destroy   = &blocks_destroy,
	{
		.func_create = &blocks_create,
		.func_set    = &blocks_setget,
		.func_get    = &blocks_setget,
		.func_delete = &blocks_delete,
		.func_move   = &blocks_move,
		.func_count  = &blocks_count
	}
};


/*
static ssize_t   itms_insert_copy   (backend_t *chain, off_t src, off_t dst, size_t size){ // {{{
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
	
	blocks_userdata *data = (blocks_userdata *)chain->userdata;
	
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
		if(map_get_block(data, src_block_vid, &src_block_roff, &block_size) != 0)
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
static ssize_t   itms_delete        (backend_t *chain, off_t start, size_t size){ // {{{
	off_t         start_real,      end_real;
	unsigned int  start_block_off, end_block_off;
	unsigned int  start_block_vid, end_block_vid, i_vid;
	unsigned int  end_block_aoff;                          // absolute offset
	off_t         block_roff;
	size_t        block_size;
	
	blocks_userdata *data = (blocks_userdata *)chain->userdata;
	
	if(map_off(data, start,        &start_block_vid, &start_real) != 0)
		return -EFAULT;
	if(map_off(data, start + size, &end_block_vid,   &end_real)   != 0){
		end_block_vid = (data->blocks_count - 1);
		end_block_off = data->block_size;
	}else{
		end_block_off   = end_real   % data->block_size;
		end_block_aoff  = (end_real - end_block_off);
	}
	start_block_off = start_real % data->block_size;
	
	for(i_vid = start_block_vid; i_vid <= end_block_vid; i_vid++){
		if(i_vid == end_block_vid){
			if(map_get_block(data, end_block_vid, &block_roff, &block_size) != 0)
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
