#include <libfrozen.h>

#define READ_PER_CALC_DEFAULT  1000

typedef struct tree_t {
	chain_t       *chain;
	
	off_t         *tof;
	size_t        *table;
	size_t        *lss;
	
	unsigned int   elements_per_level;
	unsigned int   blocks_count;
	unsigned int   nlevels;
	unsigned int   read_per_calc;
} tree_t;

typedef struct addrs_user_data {
	tree_t       *tree;

} addrs_user_data;

typedef struct block_info {
	unsigned int  real_block_off; 
	unsigned int  size;
} block_info;

static unsigned int log_any(size_t x, size_t power){
	unsigned int res   = 0;
	size_t       t     = power;
	
	while (t <= x){
		++res;
		t *= power;
	}
	return res;
}

/* size-tree {{{ */
static int      tree_reinit(tree_t *tree){ // {{{
	unsigned int      i;
	size_t            ls;
	unsigned int      nlevels;
	size_t            table_size;
	size_t           *table;
	size_t           *lss;
	off_t            *tof;
	
	buffer_t  req_buffer;
	hash_t    req_count[] = {
		{ "action", DATA_INT32(ACTION_CRWD_COUNT) },
		hash_end
	};
	buffer_init_from_bare(&req_buffer, &tree->blocks_count, sizeof(tree->blocks_count));
	if(chain_next_query(tree->chain, req_count, &req_buffer) <= 0){
		buffer_destroy(&req_buffer);
		return -1;
	}
	buffer_destroy(&req_buffer);
	
	tree->blocks_count      /= sizeof(block_info);
	
	nlevels = log_any(tree->blocks_count, tree->elements_per_level) + 1;
	if(nlevels == tree->nlevels)
		return 0;
	
	// TODO lock
	
	/* remove old data */
	if(tree->lss != NULL)
		free(tree->lss);
	if(tree->tof != NULL)
		free(tree->tof);
	if(tree->table != NULL)
		free(tree->table);
	
	if( (lss = malloc(sizeof(size_t) * nlevels)) == NULL)
		goto free;
	
	if( (tof = malloc(sizeof(off_t)  * nlevels)) == NULL)
		goto free1;
	
	table_size = 0;
	ls         = 1;
	tof[0]     = 0;
	
	for(i=1; i <= nlevels; i++){
		if(i != nlevels)
			tof[i]    = tof[i - 1] + ls;
		
		table_size       += ls;
		ls               *= tree->elements_per_level;
		lss[nlevels - i]  = ls;
	}
	table_size *= sizeof(size_t);
	
	//printf("tree_reinit: nelements: %x, nlevels: %x, table_size %d\n", (unsigned int)tree->blocks_count, nlevels, (unsigned int)table_size);
	
	if( (table = malloc(table_size)) == NULL )
		goto free2;
	
	tree->table              = table;
	tree->nlevels            = nlevels;
	tree->lss                = lss;                    // LevelSizeSizes
	tree->tof                = tof;                    // TableOFfsets
	
	// TODO unlock

	return 0;
	
free2:	free(tof);
free1:  free(lss);
free:	
	// TODO unlock
	return -1;
} // }}}
static tree_t * tree_alloc(chain_t *chain, unsigned int elements_per_level, unsigned int read_per_calc){ // {{{
	tree_t           *tree;
	
	if( (tree = calloc(1, sizeof(tree_t))) == NULL)
		return NULL;
	
	tree->nlevels            = 0;
	tree->chain              = chain;
	tree->elements_per_level = elements_per_level;
	tree->read_per_calc      = read_per_calc;
	
	if(tree_reinit(tree) != 0)
		goto free;
	
	return tree;
	
free:   free(tree);
	return NULL;
} // }}}
static void     tree_free(tree_t *tree){ // {{{
	if(tree->lss != NULL)
		free(tree->lss);
	if(tree->tof != NULL)
		free(tree->tof);
	if(tree->table != NULL)
		free(tree->table);
	
	free(tree);
} // }}}
static int      tree_recalc(tree_t *tree){ // {{{
	int           i,j;
	ssize_t       ret_size;
	unsigned int  block_size;
	unsigned int  read_size;
	buffer_t     *buffer;
	size_t       *calcs;
	size_t        blocks_left;
	unsigned int  ptr                = 0;
	unsigned int  nlevels            = tree->nlevels;
	
	if( (blocks_left = tree->blocks_count) == 0)
		return 0;
	
	calcs  = calloc(sizeof(size_t), tree->nlevels);
	buffer = buffer_alloc();
	
	while(blocks_left > 0){
		read_size = ( (blocks_left > tree->read_per_calc) ? tree->read_per_calc : blocks_left );
		
		hash_t    req_read[] = {
			{ "action", DATA_INT32(ACTION_CRWD_READ)               },
			{ "key",    DATA_OFFT (ptr * sizeof(block_info))       },
			{ "size",   DATA_INT32(read_size * sizeof(block_info)) },
			hash_end
		};
		if( (ret_size = chain_next_query(tree->chain, req_read, buffer)) <= 0)
			break;
		
		buffer_process(buffer, ret_size, 0,
			do {
				for(i=0; i < size; i += sizeof(block_info), ptr += 1){
					block_size = ((block_info *)(chunk + i))->size;
					
					//printf("block: %x size: %x\n", (unsigned int)ptr, (unsigned int)block_size);
					
					for(j=0; j < nlevels; j++){
						calcs[j] += block_size;
						
						//printf("block: [%x/%x] calcs[%x] = %x  (ptr: %x)\n",
						//	(unsigned int)ptr, (unsigned int)tree->blocks_count,
						//	(unsigned int)j, (unsigned int)calcs[j],
						//	(unsigned int)( ptr / tree->lss[j] )
						//);
						
						if( (ptr % tree->lss[j]) == tree->lss[j] - 1){
							tree->table[ tree->tof[j] + (ptr / tree->lss[j]) ]  = calcs[j];
							calcs[j]                                            = 0;
						}
					}
				}
			}while(0)
		);
		blocks_left -= ret_size / sizeof(block_info);
	}
	
	// flush
	for(j=0; j < nlevels; j++){
		/*
		printf("dumping j %x ptr: %x lss: %x [%x]=%x\n",
			j,
			(unsigned int)ptr,
			(unsigned int)tree->lss[j],
			(unsigned int)(tree->tof[j] + (ptr / tree->lss[j])),
			(unsigned int)calcs[j]
		);*/
		
		tree->table[ tree->tof[j] + (ptr / tree->lss[j]) ] = calcs[j];
	}
	
	buffer_free(buffer);
	free(calcs);
	return (blocks_left == 0) ? 0 : -1;
} // }}}
static int      tree_insert(tree_t *tree, unsigned int block_vid, unsigned int block_off, unsigned int size){ // {{{
	block_info       block;
	ssize_t          ret;
	buffer_t         req_buffer;
	
	buffer_init_from_bare(&req_buffer, &block, sizeof(block));
	
	block.real_block_off   = block_off;
	block.size             = size;
	
	hash_t    req_move[] = {
		{ "action",   DATA_INT32(ACTION_CRWD_MOVE)                     },
		{ "key_from", DATA_OFFT ((block_vid    ) * sizeof(block_info)) },
		{ "key_to",   DATA_OFFT ((block_vid + 1) * sizeof(block_info)) },
		hash_end
	};
	ret = chain_next_query(tree->chain, req_move, NULL);
	
	hash_t    req_write[] = {
		{ "action", DATA_INT32( ACTION_CRWD_WRITE              ) },
		{ "key",    DATA_OFFT ( block_vid * sizeof(block_info) ) },
		{ "size",   DATA_INT32( sizeof(block_info)             ) },
		hash_end
	};
	if( (ret = chain_next_query(tree->chain, req_write, &req_buffer)) <= 0)
		goto cleanup;
	
	ret = -1;
	if(tree_reinit(tree) != 0)
		goto cleanup;
	
	// TODO replace recalc with faster procedure which recalc only changed items instead of all
	if(tree_recalc(tree) != 0)
		goto cleanup;
	
	ret = 0;
cleanup:
	buffer_destroy(&req_buffer);
	return ret;
} // }}}
static int      tree_delete_block(tree_t *tree, unsigned int block_vid){ // {{{
	ssize_t           ret;
	
	hash_t    req_move[] = {
		{ "action",   DATA_INT32( ACTION_CRWD_MOVE                     ) },
		{ "key_from", DATA_OFFT ( (block_vid + 1) * sizeof(block_info) ) },
		{ "key_to",   DATA_OFFT ( (block_vid    ) * sizeof(block_info) ) },
		hash_end
	};
	if( (ret = chain_next_query(tree->chain, req_move, NULL)) != 0)
		return ret;
	
	hash_t    req_delete[] = {
		{ "action", DATA_INT32( ACTION_CRWD_DELETE                            ) },
		{ "key",    DATA_OFFT ( (tree->blocks_count - 1) * sizeof(block_info) ) },
		{ "size",   DATA_INT32( sizeof(block_info)                            ) },
		hash_end
	};
	if( (ret = chain_next_query(tree->chain, req_delete, NULL)) != 0)
		return ret;
	
	if(tree_reinit(tree) != 0)
		return -1;
	
	// TODO replace recalc with faster procedure which recalc only changed items instead of all
	if(tree_recalc(tree) != 0)
		return -1;
	
	return 0;
} // }}}
static int      tree_resize_block(tree_t *tree, unsigned int block_vid, unsigned int new_size){ // {{{
	unsigned int      delta;
	ssize_t           ret;
	block_info        block;
	int               j;
	buffer_t          req_buffer;
	
	buffer_init_from_bare(&req_buffer, &block, sizeof(block));
	
	/* read block_info */
	hash_t    req_read[] = {
		{ "action", DATA_INT32( ACTION_CRWD_READ               ) },
		{ "key",    DATA_OFFT ( block_vid * sizeof(block_info) ) },
		{ "size",   DATA_INT32( sizeof(block_info)             ) },
		hash_end
	};
	if( (ret = chain_next_query(tree->chain, req_read, &req_buffer)) <= 0){
		buffer_destroy(&req_buffer);
		return -1;
	}
	
	/* fix block_info */
	delta      = new_size - block.size;
	block.size = new_size; 
	
	/* write block info */
	hash_t    req_write[] = {
		{ "action", DATA_INT32( ACTION_CRWD_WRITE              ) },
		{ "key",    DATA_OFFT ( block_vid * sizeof(block_info) ) },
		{ "size",   DATA_INT32( sizeof(block_info)             ) },
		hash_end
	};
	chain_next_query(tree->chain, req_write, &req_buffer);
	
	// TODO lock
		for(j=0; j < tree->nlevels; j++){
			tree->table[ tree->tof[j] + (block_vid / tree->lss[j]) ] += delta;
		}
	// TODO unlock
	
	buffer_destroy(&req_buffer);
	return 0;
} // }}}
static int      tree_get(tree_t *tree, off_t offset, unsigned int *block_vid, off_t *real_offset){ // {{{
	unsigned int   i,j,ret;
	off_t          level_off;
	unsigned int   ptr;
	size_t         chunk_size;
	block_info     block;
	
	if(offset >= tree->table[0]) // out of range
		return -1;
	
	ret       = -1;
	level_off = 0;
	ptr       = 0;
	// TODO lock
		for(i=1; i < tree->nlevels; i++){
			for(j=0; j < tree->elements_per_level; j++, ptr++){
				chunk_size = tree->table[ tree->tof[i] + ptr];
				
				/*printf("lev: %x el: %x ptr: %x (%x < %x + %x)\n",
					i, j, (unsigned int)ptr,
					(unsigned int)offset, (unsigned int)level_off, (unsigned int)chunk_size
				);*/
				
				if(offset < level_off + chunk_size)
					break;
				
				level_off += chunk_size;
			}
			
			ptr *= tree->elements_per_level;
		}
		// last level
		
		buffer_t  req_buffer;
		buffer_init_from_bare(&req_buffer, &block, sizeof(block));
		
		/* read block_info */
		for(j=0; j < tree->elements_per_level; j++, ptr++){
			hash_t    req_read[] = {
				{ "action", DATA_INT32( ACTION_CRWD_READ               ) },
				{ "key",    DATA_OFFT ( ptr * sizeof(block_info)       ) },
				{ "size",   DATA_INT32( sizeof(block_info)             ) },
				hash_end
			};
			if(chain_next_query(tree->chain, req_read, &req_buffer) <= 0)
				break;
			
			/*printf("el: %x ptr: %x (%x < %x + %x)\n",
				j, (unsigned int)ptr,
				(unsigned int)offset, (unsigned int)level_off, (unsigned int)block.size
			);*/
			
			if(offset < level_off + block.size){
				*block_vid   = ptr;
				*real_offset = block.real_block_off + (offset - level_off);
				ret = 0;
				break;
			}
			level_off += block.size;
		}
		buffer_destroy(&req_buffer);
		
	// TODO unlock
	return ret;
} // }}}
static int      tree_get_block(tree_t *tree, unsigned int block_vid, block_info *block){ // {{{
	buffer_t         req_buffer;
	
	buffer_init_from_bare(&req_buffer, block, sizeof(block_info));
	
	hash_t    req_read[] = {
		{ "action", DATA_INT32( ACTION_CRWD_READ               ) },
		{ "key",    DATA_OFFT ( block_vid * sizeof(block_info) ) },
		{ "size",   DATA_INT32( sizeof(block_info)             ) },
		hash_end
	};
	if(chain_next_query(tree->chain, req_read, &req_buffer) <= 0)
		return -1;
	
	buffer_destroy(&req_buffer);
	return 0;
} // }}}
static int      tree_size(tree_t *tree, size_t *size){ // {{{
	*size = tree->table[0]; // root element
	return 0;
} // }}}
static unsigned int tree_blocks_count(tree_t *tree){ // {{{
	return tree->blocks_count;
} // }}}

/* }}} */

/* init {{{ */
static int addrs_init(chain_t *chain){
	addrs_user_data *user_data = calloc(1, sizeof(addrs_user_data));
	if(user_data == NULL)
		return -ENOMEM;
	
	chain->user_data = user_data;
	
	return 0;
}

static int addrs_destroy(chain_t *chain){
	addrs_user_data *data = (addrs_user_data *)chain->user_data;
	
	tree_free(data->tree);
	
	free(chain->user_data);
	
	chain->user_data = NULL;
	return 0;
}

static int addrs_configure(chain_t *chain, setting_t *config){
	char             *elements_per_level_str;
	unsigned int      elements_per_level;
	char             *read_per_calc_str;
	unsigned int      read_per_calc;
	
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	if( (elements_per_level_str   = setting_get_child_string(config, "per-level")) == NULL)
		return_error(-EINVAL, "chain 'blocks-address' variable 'per-level' not set");
	
	if( (elements_per_level       = strtoul(elements_per_level_str, NULL, 10)) <= 1)
		return_error(-EINVAL, "chain 'blocks-address' variable 'per-level' invalid");
	
	read_per_calc = READ_PER_CALC_DEFAULT;
	if( (read_per_calc_str        = setting_get_child_string(config, "recalc-read-size")) != NULL){
		if( (read_per_calc    = strtoul(elements_per_level_str, NULL, 10)) < 1)
			read_per_calc = READ_PER_CALC_DEFAULT;
	}
	
	if( (data->tree = tree_alloc(chain, elements_per_level, read_per_calc)) == NULL)
		goto free1;
	
	if(tree_recalc(data->tree) != 0)
		return_error(-EINVAL, "chain 'blocks-address' tree recalc failed\n");
	
	return 0;
	
free1:  
	return_error(-ENOMEM, "chain 'blocks-address' no memory\n"); 
}
/* }}} */

static ssize_t addrs_set(chain_t *chain, request_t *request, buffer_t *buffer){
	unsigned int      block_vid;
	unsigned int      insert       = 1;
	hash_t           *r_block_size, *r_block_vid, *r_block_off;
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	if( (r_block_size = hash_find_typed_value(request, TYPE_INT32, "block_size")) == NULL) 
		return -EINVAL;
	
	if( (r_block_vid  = hash_find_typed_value(request, TYPE_INT32, "block_vid"))  == NULL)
		block_vid = tree_blocks_count(data->tree);
	else
		block_vid = HVALUE(r_block_vid, unsigned int);
	
	if( (r_block_off  = hash_find_typed_value(request, TYPE_INT32, "block_off"))  == NULL)
		insert = 0;
	
	if(insert == 0){
		return tree_resize_block(data->tree, block_vid, HVALUE(r_block_size, unsigned int));
	}
	return tree_insert(data->tree, block_vid, HVALUE(r_block_off, unsigned int), HVALUE(r_block_size, unsigned int));
}

// TODO custom functions to chain
// translate_key  - get("offset")
// get block_size - get("blocks", "block_vid")

static ssize_t addrs_get(chain_t *chain, request_t *request, buffer_t *buffer){
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	off_t             real_key;
	unsigned int      block_vid;
	block_info        block;
	ssize_t           buf_ptr = 0;
	hash_t           *r_virt_key, *r_block_vid;
	
	if(hash_find_value(request, "blocks") == NULL){
		if( (r_virt_key = hash_find_typed_value(request, TYPE_OFFT, "offset")) == NULL)
			return -EINVAL;
		
		if(tree_get(data->tree, HVALUE(r_virt_key, off_t), &block_vid, &real_key) != 0)
			return -EFAULT;
		
		buf_ptr += buffer_write(buffer, buf_ptr, &real_key,  sizeof(real_key));
		buf_ptr += buffer_write(buffer, buf_ptr, &block_vid, sizeof(block_vid));
	}else{
		if( (r_block_vid = hash_find_typed_value(request, TYPE_INT32, "block_vid")) == NULL)
			return -EINVAL;
		
		if(tree_get_block(data->tree, HVALUE(r_block_vid, unsigned int), &block) != 0)
			return -EFAULT;
		
		buf_ptr += buffer_write(buffer, buf_ptr, &block.size,           sizeof(block.size));
		buf_ptr += buffer_write(buffer, buf_ptr, &block.real_block_off, sizeof(block.real_block_off));
	}
	return buf_ptr;
}

static ssize_t addrs_delete(chain_t *chain, request_t *request, buffer_t *buffer){
	hash_t           *r_block_vid;
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	if( (r_block_vid = hash_find_typed_value(request, TYPE_INT32, "block_vid")) == NULL)
		return -EINVAL;
	
	return tree_delete_block(data->tree, HVALUE(r_block_vid, unsigned int));
}

static ssize_t addrs_count(chain_t *chain, request_t *request, buffer_t *buffer){
	size_t            units_count; 
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	if(hash_find_value(request, "blocks") == NULL){
		if(tree_size(data->tree, &units_count) != 0)
			return -EINVAL;
	}else{
		units_count = tree_blocks_count(data->tree);
	}
	
	return buffer_write(buffer, 0, &units_count, sizeof(units_count));
}

static chain_t chain_addrs = {
	"blocks-address",
	CHAIN_TYPE_CRWD,
	&addrs_init,
	&addrs_configure,
	&addrs_destroy,
	{{
		.func_set    = &addrs_set,
		.func_get    = &addrs_get,
		.func_delete = &addrs_delete,
		.func_count  = &addrs_count
	}}
};
CHAIN_REGISTER(chain_addrs)

