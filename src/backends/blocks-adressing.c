#include <libfrozen.h>

#define READ_PER_CALC  1000

typedef struct tree_t {
	chain_t       *chain;
	size_t        *table;
	size_t         elements_per_level;
	unsigned int   blocks_count;
	unsigned int   nlevels;
	size_t        *lss;
	off_t         *tof;
} tree_t;

typedef struct addrs_user_data {
	request_t    *request_count;
	request_t    *request_read;
	request_t    *request_write;
	request_t    *request_move;
	buffer_t     *buffer_read;
	buffer_t     *buffer_count;
	data_t       *request_write_data;
	tree_t       *tree;

} addrs_user_data;

typedef struct block_info {
	unsigned int  real_block_off; 
	unsigned int  size;
} block_info;

static void       m_count(chain_t *chain, size_t *p_size){
	ssize_t           ret;
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	ret = chain_next_query(chain, data->request_count, data->buffer_count);
	if(ret > 0){
		buffer_read_flat(data->buffer_count, ret, p_size, sizeof(size_t));
		return;
	}
	*p_size = 0;
}

static buffer_t * m_read(chain_t *chain, off_t ptr, size_t size){
	ssize_t           ret;
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	hash_set(data->request_read, "key",  TYPE_INT64, &ptr);
	hash_set(data->request_read, "size", TYPE_INT32, &size);
	
	ret = chain_next_query(chain, data->request_read, data->buffer_read);
	if(ret > 0){
		return data->buffer_read;
	}
	return NULL;
}
static ssize_t   m_write(chain_t *chain, off_t ptr, void *buf, size_t size){
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	hash_set(data->request_write, "key",  TYPE_INT64, &ptr);
	hash_set(data->request_write, "size", TYPE_INT32, &size);
	memcpy(data->request_write_data + 4, buf, (size > 8) ? 8 : size);
	
	return chain_next_query(chain, data->request_write, NULL);
}


static ssize_t   m_move(chain_t *chain, off_t from, off_t to, size_t size){
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	hash_set(data->request_move, "key_from",  TYPE_INT64, &from);
	hash_set(data->request_move, "key_to",    TYPE_INT64, &to);
	if(size != (size_t) -1)
		hash_set(data->request_move, "size", TYPE_INT32, &size);
	
	return chain_next_query(chain, data->request_move, NULL);
}


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
	
	nlevels = log_any(tree->blocks_count, tree->elements_per_level) + 1;
	if(nlevels == tree->nlevels)
		return 0;
	
	/* remove old data */
	if(tree->lss != NULL)
		free(tree->lss);
	if(tree->tof != NULL)
		free(tree->tof);
	if(tree->table != NULL)
		free(tree->table);
	
	if( (lss = calloc(sizeof(size_t),  nlevels)) == NULL)
		goto free;
	
	if( (tof = calloc(sizeof(off_t),   nlevels)) == NULL)
		goto free1;
	
	table_size = 0;
	ls         = 1;
	
	for(i=1; i <= nlevels; i++){
		if(i != nlevels)
			tof[i]    = tof[i - 1] + ls;
		
		ls               *= tree->elements_per_level;
		table_size       += ls;
		lss[nlevels - i]  = ls;
	}
	table_size *= sizeof(size_t);
	
	//printf("tree_reinit: nelements: %x, nlevels: %x, table_size %x\n", (unsigned int)tree->blocks_count, nlevels, (unsigned int)table_size);
	
	if( (table = calloc(1, table_size)) == NULL )
		goto free2;
	
	tree->table              = table;
	tree->nlevels            = nlevels;
	tree->lss                = lss;                    // LevelSizeSizes
	tree->tof                = tof;                    // TableOFfsets
	
	return 0;
	
free2:	free(tof);
free1:  free(lss);
free:	
	return -1;
} // }}}
static tree_t * tree_alloc(chain_t *chain, unsigned int elements_per_level){ // {{{
	tree_t           *tree;
	
	if( (tree = calloc(1, sizeof(tree_t))) == NULL)
		return NULL;
	
	tree->nlevels            = 0;
	tree->chain              = chain;
	tree->elements_per_level = elements_per_level;
	
	m_count(chain, &tree->blocks_count);
	tree->blocks_count /= sizeof(block_info);
	
	if(tree_reinit(tree) != 0)
		goto free;
	
	return tree;
	
free:   free(tree);
	return NULL;
} // }}}
static int      tree_recalc(tree_t *tree){ // {{{
	size_t        read_size;
	unsigned int  block_size;
	buffer_t     *buffer;
	int           ret                = -1;
	unsigned int  nlevels            = tree->nlevels;
	size_t        blocks_left        = tree->blocks_count;
	off_t         ptr                = 0;
	int           i,j;
	
	if(blocks_left == 0)
		return 0;
	
	size_t       *calcs              = calloc(sizeof(size_t), tree->nlevels);
		
	while(blocks_left > 0){
		read_size    = ( (blocks_left > READ_PER_CALC) ? READ_PER_CALC : blocks_left );
		blocks_left -= read_size;
		read_size   *= sizeof(block_info); 
		
		buffer    = m_read(tree->chain, ptr, read_size);
		if(buffer == NULL)
			break;
		
		buffer_read(buffer, read_size,
			do {
				for(i=0; i < size; i += sizeof(block_info), ptr += 1){
					block_size = ((block_info *)(chunk + i))->size;
					
					//printf("block: %x size: %x\n", (unsigned int)ptr, (unsigned int)block_size);
					
					for(j=0; j < nlevels; j++){
						calcs[j] += block_size;
						
						//printf("block: %x calcs[%x] = %x  (ptr: %x)\n",
						//	(unsigned int)ptr, (unsigned int)j, (unsigned int)calcs[j],
						//	(unsigned int)( ptr / tree->lss[j] )
						//);
						
						if( (ptr % tree->lss[j]) == tree->lss[j] - 1){
							//printf("dumping %x %x\n", (unsigned int)ptr, (unsigned int)tree->lss[j]);
							
							tree->table[ tree->tof[j] + (ptr / tree->lss[j]) ]  = calcs[j];
							calcs[j]                                            = 0;
						}
					}
				}
			}while(0)
		);
		
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
		ret = 0;
	}
	
	free(calcs);
	return ret;
} // }}}
static int      tree_insert(tree_t *tree, unsigned int block_vid, unsigned int block_off, unsigned int size){ // {{{
	block_info  block;
	ssize_t     ret;
	
	block.real_block_off = block_off;
	block.size           = size;
	
	ret = m_move(
		tree->chain,
		(block_vid    ) * sizeof(block_info),
		(block_vid + 1) * sizeof(block_info),
		-1
	);
	
	ret = m_write(tree->chain, block_vid * sizeof(block_info), &block, sizeof(block_info));
	if(ret <= 0)
		return ret;
	
	tree->blocks_count++;
	if(tree_reinit(tree) != 0)
		return -1;
	
	// TODO remove recalc with faster procedure which recalc only changed items, instead of all
	if(tree_recalc(tree) != 0)
		return -1;
	
	return 0;
} // }}}

static int      tree_delete_block(tree_t *tree, unsigned int block_vid){ // {{{
	ssize_t     ret;
	
	ret = m_move(
		tree->chain,
		(block_vid + 1) * sizeof(block_info),
		(block_vid    ) * sizeof(block_info),
		-1
	);
	
	tree->blocks_count--;
	if(tree_reinit(tree) != 0)
		return -1;
	
	// TODO remove recalc with faster procedure which recalc only changed items, instead of all
	if(tree_recalc(tree) != 0)
		return -1;
	
	return 0;
} // }}}

static int      tree_resize_block(tree_t *tree, unsigned int block_vid, unsigned int new_size){ // {{{
	unsigned int   delta;
	buffer_t      *buffer;
	block_info     block;
	int            j;
	
	/* read block_info */
	buffer = m_read(tree->chain, block_vid * sizeof(block_info), sizeof(block_info));
	if(buffer == NULL)
		return -1;
		
	buffer_read(buffer, sizeof(block_info),
		memcpy(&block, chunk, size)
	);
	
	/* fix block_info */
	delta      = new_size - block.size;
	block.size = new_size; 
	
	/* write block info */
	m_write(tree->chain, block_vid * sizeof(block_info), &block, sizeof(block_info));
	
	// TODO lock
		for(j=0; j < tree->nlevels; j++){
			tree->table[ tree->tof[j] + (block_vid / tree->lss[j]) ] += delta;
		}
	// TODO unlock
	return 0;
} // }}}
static int      tree_get(tree_t *tree, off_t offset, off_t *real_offset){ // {{{
	unsigned int   i,j,ret;
	off_t          level_off;
	off_t          ptr;
	size_t         chunk_size;
	buffer_t      *buffer;
	block_info     block;
	
	if(offset >= tree->table[0]) // out of range
		return -1;
	
	ret       = -1;
	level_off = 0;
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
		
		/* read block_info */
		for(j=0; j < tree->elements_per_level; j++, ptr++){
			buffer = m_read(tree->chain, ptr * sizeof(block_info), sizeof(block_info));
			if(buffer == NULL)
				goto unlocks;
				
			buffer_read(buffer, sizeof(block_info),
				memcpy(&block, chunk, size)
			);
			/*printf("el: %x ptr: %x (%x < %x + %x)\n",
				j, (unsigned int)ptr,
				(unsigned int)offset, (unsigned int)level_off, (unsigned int)block.size
			);*/
			if(offset < level_off + block.size){
				*real_offset = block.real_block_off + (offset - level_off);
				ret = 0;
				goto unlocks;
			}
			level_off += block.size;
		}
unlocks:	
	// TODO unlock
	return ret;
} // }}}
static int      tree_size(tree_t *tree, size_t *size){ // {{{
	*size = tree->table[0]; // root element
	return 0;
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
	//addrs_user_data *data = (addrs_user_data *)chain->user_data;
	
	free(chain->user_data);
	
	chain->user_data = NULL;
	return 0;
}

static int addrs_configure(chain_t *chain, setting_t *config){
	char      *elements_per_level_str;
	size_t     elements_per_level;
	int        action;
	off_t      key;
	size_t     size;
	char       value_data[] = "\x0c\x00\x00\x00" "\x00\x00\x00\x00\x00\x00\x00\x00";
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	if( (elements_per_level_str   = setting_get_child_string(config, "per-level")) == NULL)
		return_error(-EINVAL, "chain 'blocks-address' variable 'per-level' not set");
	
	if( (elements_per_level       = strtoul(elements_per_level_str, NULL, 10)) <= 1)
		return_error(-EINVAL, "chain 'blocks-address' variable 'per-level' invalid");
	
	if( (data->request_count = hash_new()) == NULL)
		goto free;
	
	if( (data->request_read  = hash_new()) == NULL)
		goto free1;
	
	if( (data->request_write = hash_new()) == NULL)
		goto free2;
	
	if( (data->request_move = hash_new()) == NULL)
		goto free3;
	
	if( (data->buffer_read = buffer_alloc()) == NULL)
		goto free4;
	
	if( (data->buffer_count = buffer_alloc()) == NULL)
		goto free5;
	
	action = ACTION_CRWD_COUNT;
	hash_set(data->request_count,  "action", TYPE_INT32, &action);
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
	hash_get_any(data->request_write, "value", NULL, &data->request_write_data, NULL);
	
	size   = 1;
	hash_set(data->request_read,   "size",   TYPE_INT32, &size);
	hash_set(data->request_write,  "size",   TYPE_INT32, &size);
	
	if( (data->tree = tree_alloc(chain, elements_per_level)) == NULL)
		goto free6;
	
	if(tree_recalc(data->tree) != 0)
		return_error(-EINVAL, "chain 'blocks-address' tree recalc failed\n");
	
	return 0;
	
free6:  buffer_free(data->buffer_count);
free5:  buffer_free(data->buffer_read);
free4:  hash_free(data->request_move);
free3:	hash_free(data->request_write);
free2:	hash_free(data->request_read);
free1:  hash_free(data->request_count);
free:	return_error(-ENOMEM, "chain 'blocks-address' no memory\n"); 
}
/* }}} */

static ssize_t addrs_set(chain_t *chain, request_t *request, buffer_t *buffer){
	unsigned int      block_vid;
	unsigned int      block_off;
	unsigned int      block_size;
	unsigned int      insert       = 1;
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	if(hash_get_copy (request, "block_size",  TYPE_INT32, &block_size,  sizeof(block_size)) != 0) 
		return -EINVAL;
	
	if(hash_get_copy (request, "block_vid",   TYPE_INT32, &block_vid,   sizeof(block_vid))  != 0)
		block_vid = data->tree->blocks_count; // TODO func
	
	if(hash_get_copy (request, "block_off",   TYPE_INT32, &block_off,   sizeof(block_off))  != 0)
		insert = 0;
	
	if(insert == 0)
		return tree_resize_block(data->tree, block_vid, block_size);
	
	return tree_insert(data->tree, block_vid, block_off, block_size);
}

static ssize_t addrs_get(chain_t *chain, request_t *request, buffer_t *buffer){
	addrs_user_data *data = (addrs_user_data *)chain->user_data;
	off_t             virt_key;
	off_t             real_key;
	
	if(hash_get_copy (request, "offset", TYPE_INT64, &virt_key, sizeof(virt_key)) != 0)
		return -EINVAL;
	
	if(tree_get(data->tree, virt_key, &real_key) != 0)
		return -EINVAL;
	
	buffer_write(buffer, sizeof(real_key),
		memcpy(chunk, &real_key, size)
	);
	
	return sizeof(real_key);
}

static ssize_t addrs_delete(chain_t *chain, request_t *request, buffer_t *buffer){
	unsigned int      block_vid;
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	if(hash_get_copy (request, "block_vid",   TYPE_INT32, &block_vid,   sizeof(block_vid))  != 0)
		return -EINVAL;
	
	return tree_delete_block(data->tree, block_vid);
}

static ssize_t addrs_count(chain_t *chain, request_t *request, buffer_t *buffer){
	size_t            elements_count; 
	addrs_user_data  *data = (addrs_user_data *)chain->user_data;
	
	if(tree_size(data->tree, &elements_count) != 0)
		return -EINVAL;
	
	buffer_write(buffer, sizeof(elements_count),
		memcpy(chunk, &elements_count + offset, size)
	);
	
	return sizeof(elements_count);
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

