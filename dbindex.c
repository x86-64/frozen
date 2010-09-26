#include <memcachedb.h>
#include <list.h>
#include <dbmap.h>
#include <dbindex.h>

#define SEARCH_FOUND      0
#define SEARCH_NOT_FOUND  1 
#define DB_SUCCESS        0
#define DB_ERROR          1

// TODO rewrite iblock from unsigned short to structure
// TODO rewrite code to use vm wrappers
// TODO rewrite PRIMARY code to use vm wrappers
// TODO fix binary search http://habrahabr.ru/blogs/algorithm/91605/

/* DBIndex IBlocks {{{1 */

static char *dbindex_iblock_get_page(dbindex *index, unsigned short iblock){
	return index->file.data + index->data_offset + (iblock - 1) * index->page_size;
}

/* iblock locks {{{2 */
static void dbindex_iblock_lock_init(pthread_rwlock_t **iblock_locks, unsigned short iblock){
	if(iblock_locks == NULL)
		return;

	pthread_rwlock_t *new_lock = malloc(sizeof(pthread_rwlock_t));
	
	pthread_rwlock_init(new_lock, NULL);

	iblock_locks[iblock] = new_lock;
}

static void dbindex_iblock_lock_free(pthread_rwlock_t **iblock_locks, unsigned short iblock){
	free( iblock_locks[iblock] );
}

static void dbindex_iblock_wrlock(dbindex* index, unsigned short iblock){
	if(iblock > index->iblock_last){
		printf("wrlock, table have broken block count! (%d out of %d)\n", iblock, index->iblock_last);
		return;
	}
	pthread_rwlock_wrlock( index->iblock_locks[iblock] );
}

static void dbindex_iblock_rdlock(dbindex* index, unsigned short iblock){
	if(iblock > index->iblock_last){
		printf("rdlock, table have broken block count! (%d out of %d)\n", iblock, index->iblock_last);
		return;
	}
	pthread_rwlock_rdlock( index->iblock_locks[iblock] );
}

static void dbindex_iblock_unlock(dbindex* index, unsigned short iblock){
	if(iblock > index->iblock_last){
		printf("unlock, table have broken block count! (%d out of %d)\n", iblock, index->iblock_last);
		return;
	}
	pthread_rwlock_unlock( index->iblock_locks[iblock] );
}
/* }}}2 */
/* iblock start {{{2 */
// TODO refactor free table routines

static void  dbindex_iblock_init_start(dbindex *index, unsigned short iblock, unsigned int value){
	char *free_table;
	char *page_offset = dbindex_iblock_get_page(index, iblock);
	
	free_table = index->file.data + DBINDEX_DATA_OFFSET + 0x20000;
	free_table[(iblock - 1) / 8] &= ~( 1 << (7 - (iblock - 1) % 8) ); // set not full
	
	*((unsigned int *)page_offset) = value;
}

static unsigned int dbindex_iblock_get_start(dbindex *index, unsigned short iblock, char **ppage_data_start, char **ppage_data_end){
	char         *free_table;
	bool          is_full;
	
	unsigned int  free_size = 0;
	char         *page_data_start = dbindex_iblock_get_page(index, iblock); 
	char         *page_data_end   = page_data_start + index->page_size;
	
	free_table = index->file.data + DBINDEX_DATA_OFFSET + 0x20000;
	
	is_full = (free_table[(iblock - 1) / 8] >> (7 - ((iblock - 1) % 8))) & 0x1;

	if(!is_full){
		free_size = *((unsigned int *)page_data_start);
		page_data_start += free_size;
	}
	*ppage_data_start = page_data_start;
	*ppage_data_end   = page_data_end;
	
	return free_size;
}

static void  dbindex_iblock_set_start(dbindex *index, unsigned short iblock, unsigned int delta){
	unsigned int  page_len;
	char         *page_offset;
	char         *page_data_start;
	char         *page_data_end;
	char         *free_table;

	page_len    = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
	page_offset = (page_data_start - page_len);
	
	free_table  = index->file.data + DBINDEX_DATA_OFFSET + 0x20000;
	
	page_data_start -= delta;

	if(page_data_start == page_offset){
		free_table[(iblock - 1) / 8] |=  ( 1 << (7 - (iblock - 1) % 8) ); // set full
	}else{
		free_table[(iblock - 1) / 8] &= ~( 1 << (7 - (iblock - 1) % 8) ); // set not full
		*(unsigned int *)page_offset = page_data_start - page_offset;
	}
}
/* }}}2 */

/* dbindex_iblock_search
 *
 * returns:
 *     0 - item pointed by *new_key found at pitem_offset address
 *     1 - item not found, pitem_offset points to address there this item can be, page is full
 *     2 - item not found, pitem_offset points to address there this item can be, page is not full
*/

static int   dbindex_iblock_search(dbindex *index, unsigned short iblock, char **pitem_offset, char *new_key){
	unsigned int  ret;
	char         *item_offset;
	char         *page_data_start;
	char         *page_data_end;
	
	ret = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);

	/* binary search */
	char          *range_start = page_data_start;
	char          *range_end   = page_data_end;
	unsigned long  range_elements;
	int            cret;
	
	
	if(range_start == range_end){
		*pitem_offset = range_end - index->item_size;
		return 2;
	}
	
	while(1){
		range_elements = (range_end - range_start) / index->item_size;
		item_offset    = range_start + (range_elements / 2) * index->item_size;
		
/*		printf("loop %llx %llx %llx-%llx %x %x\n",
			range_elements,
			item_offset - index->file.data,
			range_start - index->file.data,
			range_end   - index->file.data,
			*(unsigned int *)item_offset,
			*(unsigned int *)new_key
		);*/
	
		cret = memcmp(item_offset, new_key, index->key_len);
		if(cret == 0){
			*pitem_offset = item_offset;
			return 0;
		}else if(cret < 0){
			range_end   = item_offset;
		}else{ // if(ret > 0){
			range_start = item_offset;
		}
		
		if(range_elements == 1){
			*pitem_offset = range_end - index->item_size;
			
			return (ret == 0) ? 1 : 2;
		}
	}
}

static unsigned short dbindex_iblock_new(dbindex *index){
	unsigned short  iblock;
	char           *iblock_offset;
	unsigned long   ret;
	
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
		case DBINDEX_TYPE_INDEX:

			ret = dbmap_expand(&index->file, index->page_size);
			if(ret == -1){
				return 0; //error
			}

			iblock = ++index->iblock_last;
			
			dbindex_iblock_init_start (index, iblock, index->page_size);
			dbindex_iblock_lock_init  (index->iblock_locks, iblock);
			break;
	};
	
	list_iter(&index->cursors, &dbindex_iter_cursors_recalc, NULL, NULL);
	
	return iblock;
}

static int dbindex_iblock_get_ourelements(dbindex *index, unsigned short iblock, char *our_element, char **our_elements_start, char **our_elements_end){
	int    no_our_elements = 0;
	char  *search_element     = zalloc(index->item_size);
				
	search_element[0] = our_element[0];
	search_element[1] = our_element[1];
	if(dbindex_iblock_search(index, iblock, our_elements_end, search_element) != 0){
		if(memcmp(search_element, *our_elements_end, 2) != 0){
			no_our_elements = 1;
		}
	}
			
	search_element[2] = (char)0xFF;
	search_element[3] = (char)0xFF;
	if(dbindex_iblock_search(index, iblock, our_elements_start, search_element) != 0){
		*our_elements_start += index->item_size;
		if(memcmp(search_element, *our_elements_start, 2) != 0){
			no_our_elements = 1;
		}
	}
			
	free(search_element);
	return no_our_elements;
}

static void dbindex_iblock_move_elements(dbindex *index, unsigned short iblock_old, unsigned short iblock_new, char *elements_start, char *elements_end){
	unsigned long  iblock_old_free;
	char          *iblock_old_data_start;
	char          *iblock_old_data_end;
	char          *iblock_new_data_start;
	char          *iblock_new_data_end;
			
	iblock_old_free = dbindex_iblock_get_start(index, iblock_old, &iblock_old_data_start, &iblock_old_data_end);
	                  dbindex_iblock_get_start(index, iblock_new, &iblock_new_data_start, &iblock_new_data_end);
	
	unsigned long  items_len = elements_end - elements_start + index->item_size;
	char          *items_pos = iblock_new_data_end - items_len;
		
	if(items_len > 0){
		memcpy(items_pos, elements_start, items_len);
	
		dbindex_iblock_set_start(index, iblock_new, items_len);
		
		memmove(iblock_old_data_start + items_len, iblock_old_data_start, elements_start - iblock_old_data_start);
		memset (iblock_old_data_start, 0, items_len); // NOTE can remove

		dbindex_iblock_init_start(index, iblock_old, iblock_old_free + items_len);
	}
}

static unsigned int dbindex_iblock_insert(dbindex *index, unsigned short iblock, unsigned int search_res, char *item_offset, char *new_key){
	char          *page_data_start;
	char          *page_data_end;
	
	if(search_res == 0){
		// found existing item
		
		memcpy(item_offset, new_key, index->item_size);
	}else if(search_res == 2){
		// insert new
			
		dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
		dbindex_iblock_set_start(index, iblock, index->item_size);
				
		if(item_offset >= page_data_start)
			memcpy(
				page_data_start - index->item_size,
				page_data_start,
				item_offset - page_data_start + index->item_size
			);
		
		memcpy(item_offset, new_key, index->item_size);	
	}
	
	return search_res;
}

static char* dbindex_iblock_pop(dbindex *index, unsigned short iblock){
	char  *page_data_start;
	char  *page_data_end;
	
	dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
	
	if(page_data_start == page_data_end)
		return NULL;
	
	char *new_key = zalloc(index->item_size);
	memcpy(new_key, page_data_start, index->item_size);
	
	dbindex_iblock_set_start(index, iblock, -index->item_size);
	
	return new_key;
}

static unsigned int dbindex_iblock_delete(dbindex *index, unsigned short iblock, char *item_offset){
	char         *page_data_start;
	char         *page_data_end;
	unsigned int  free_space;
	
	free_space = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
	
	if(item_offset > page_data_start)
		memmove(
			page_data_start + index->item_size,
			page_data_start,
			item_offset - page_data_start
		);
	free_space += index->item_size;
	dbindex_iblock_init_start(index, iblock, free_space);
	
	return free_space;
}
/* }}}1 */
/* DBIndex Virtual mapping {{{1 */

static unsigned short dbindex_vm_virt2real(dbindex *index, unsigned short iblock_virt){
	return ((unsigned short *)(index->file.data + DBINDEX_DATA_OFFSET))[iblock_virt - 1];
}

static void dbindex_vm_insert_page(dbindex *index, unsigned short iblock_virt, unsigned short iblock_real){
	unsigned short *mapping;
	mapping = &(((unsigned short *)(index->file.data + DBINDEX_DATA_OFFSET))[iblock_virt - 1]);
	
	memmove(
		mapping + 1,
		mapping,
		(0xFFFF - iblock_virt) * sizeof(short)
	);
	*mapping = iblock_real;
}

static void dbindex_vm_delete_page(dbindex *index, unsigned short iblock_virt){
	unsigned short *mapping;
	unsigned short  iblock_del_real;
	unsigned short  iblock_last_virt;
	char           *iblock_del_start;
	char           *iblock_last_start;

	if(index->iblock_last == 1) // leave last block
		return;

	mapping         = &(((unsigned short *)(index->file.data + DBINDEX_DATA_OFFSET))[iblock_virt - 1]);
	iblock_del_real = *mapping;
	
	memmove(
		mapping,
		mapping + 1,
		(0xFFFF - iblock_virt) * sizeof(short)
	);
		
	if(iblock_del_real != index->iblock_last){
		iblock_last_start = dbindex_iblock_get_page(index, index->iblock_last); 
		iblock_del_start  = dbindex_iblock_get_page(index, iblock_del_real);
		
		memcpy(iblock_del_start, iblock_last_start, index->page_size);
		
		mapping = ((unsigned short *)(index->file.data + DBINDEX_DATA_OFFSET));
		
		for(iblock_last_virt=0; iblock_last_virt<=0xFFFF; iblock_last_virt++){
			if(mapping[iblock_last_virt] == index->iblock_last){
				mapping[iblock_last_virt] = iblock_del_real;
				break;
			}
		}
	}
	index->iblock_last--;
	
	dbmap_shrink(&index->file, index->page_size);
}

static unsigned int dbindex_vm_item_insert(vm_cursor *cursor, void *data){
	unsigned int   iblock_free;
	char          *iblock_dstart;
	char          *iblock_dend;
	
	if(cursor == NULL)
		return DB_ERROR;
	
	if(cursor->real_iblock == 0){
		if( (cursor->real_iblock = dbindex_iblock_new(cursor->index)) == 0)
			return DB_ERROR;
		
		dbindex_vm_insert_page(cursor->index, 1, cursor->real_iblock);
	
		iblock_free = dbindex_iblock_get_start(cursor->index, cursor->real_iblock, &iblock_dstart, &iblock_dend);
		
		cursor->real_ptr = iblock_dend - cursor->index->item_size;
	}else{
		iblock_free = dbindex_iblock_get_start(cursor->index, cursor->real_iblock, &iblock_dstart, &iblock_dend);
		if(iblock_free == 0){
			short          iblock_new_virt;
			short          iblock_new_real;
			unsigned int   iblock_new_free;
			char          *iblock_new_dstart;
			char          *iblock_new_dend;
				
			iblock_new_virt = cursor->virt_iblock - 1;
			iblock_new_real = dbindex_vm_virt2real(cursor->index, iblock_new_virt); 
			iblock_new_free = dbindex_iblock_get_start(cursor->index, iblock_new_real, &iblock_new_dstart, &iblock_new_dend);
			
			if(iblock_new_free == 0){
				if( (iblock_new_real = dbindex_iblock_new(cursor->index)) == 0)
					return DB_ERROR;
				
				iblock_new_free = dbindex_iblock_get_start(cursor->index, iblock_new_real, &iblock_new_dstart, &iblock_new_dend);
				
				dbindex_vm_insert_page(cursor->index, cursor->virt_iblock, iblock_new_real);
			}
			
			char *poped_item;
			
			poped_item = dbindex_iblock_pop(cursor->index, cursor->real_iblock);
			
			dbindex_iblock_insert(cursor->index, iblock_new_real, 2, iblock_new_dstart - cursor->index->item_size, poped_item);
			
			free(poped_item);
		}
	}
	dbindex_iblock_insert(cursor->index, cursor->real_iblock, 2, cursor->real_ptr, data);
	
	return DB_SUCCESS;
}

static unsigned int dbindex_vm_item_delete(vm_cursor *cursor){
	unsigned int ret;
	
	ret = dbindex_iblock_delete(cursor->index, cursor->real_iblock, cursor->real_ptr);
	if(ret == cursor->index->page_size){
		dbindex_vm_delete_page(cursor->index, cursor->virt_iblock);
	}
	
	return 0;
}

/* }}}1 */
/* DBIndex virtual mapping cursors {{{1 */
char* dbindex_vm_cursor_seek(vm_cursor *c, unsigned long long value, int whence){
	unsigned long long   addr_diff;
	unsigned long long   virt_max;
	char                *page_data_end;
	char                *page_data_start;
	
	if(c->real_iblock != 0)
		dbindex_iblock_unlock(c->index, c->real_iblock);
	
	virt_max = c->index->iblock_last * c->index->page_size;
	
	switch(whence){
		case SEEK_SET:
			c->virt_addr  = value;
			break;
		case SEEK_CUR:
			c->virt_addr += value;
			break;
		case SEEK_END:
			c->virt_addr  = virt_max + value;
			break;
	};
	
	//if(c->virt_addr >= virt_max)
	//	return NULL;
	
	c->virt_iblock = (c->virt_addr / c->index->page_size) + 1;
	c->real_iblock = dbindex_vm_virt2real(c->index, c->virt_iblock);
	
	//printf("virt %d real %d\n", c->virt_iblock, c->real_iblock);
	if(c->real_iblock == 0){
		c->real_ptr = c->base_addr + c->virt_addr;
		return NULL;
	}
	
	c->real_addr   = c->virt_addr + (c->real_iblock - c->virt_iblock) * c->index->page_size;
	c->real_ptr    = c->base_addr + c->real_addr;
	
	c->page_len    = dbindex_iblock_get_start(c->index, c->real_iblock, &page_data_start, &page_data_end);
		
	if(c->rw & CURSOR_WRITE){
		dbindex_iblock_wrlock(c->index, c->real_iblock);
	}else{
		dbindex_iblock_rdlock(c->index, c->real_iblock);
	}
	
	if(c->real_ptr < page_data_start){
		if((c->rw & CURSOR_FIXED) == 0){
			addr_diff = (page_data_start - c->real_ptr);
			
			c->real_addr += addr_diff;
			c->virt_addr += addr_diff;
			c->real_ptr  += addr_diff;
		}
	}
	
	return c->real_ptr;
}

// return real_ptr on success, NULL on error
char* dbindex_vm_cursor_next(vm_cursor *cursor){
	return dbindex_vm_cursor_seek(cursor, cursor->index->item_size, SEEK_CUR);
}

// return real_ptr on success, NULL on error
char *dbindex_vm_cursor_prev(vm_cursor *cursor){
	char *real_ptr1 = cursor->real_ptr;
	char *real_ptr2 = dbindex_vm_cursor_seek(cursor, - (unsigned long long)cursor->index->item_size, SEEK_CUR);
	
	if(real_ptr1 == real_ptr2)
		return NULL;
	return real_ptr2;
}

// return real_ptr on success, NULL on error
char *dbindex_vm_cursor_recalc(vm_cursor *cursor){
	return dbindex_vm_cursor_seek(cursor, 0, SEEK_CUR);
}

static void* dbindex_iter_cursors_recalc(void *cursor, void *null1, void *null2){
	((vm_cursor *)cursor)->base_addr = ((vm_cursor *)cursor)->index->file.data + ((vm_cursor *)cursor)->index->data_offset;
	dbindex_vm_cursor_recalc((vm_cursor *)cursor);
}

vm_cursor* dbindex_vm_cursor_new(dbindex *index, int type, unsigned long value, int whence){
	vm_cursor *cursor = (vm_cursor *)zalloc(sizeof(vm_cursor));
	
	cursor->base_addr = index->file.data + index->data_offset;
	cursor->index     = index;
	cursor->rw        = type;
	
	dbindex_vm_cursor_seek(cursor, value, whence);
	
	list_push(&index->cursors, cursor);

	return cursor;
}

void dbindex_vm_cursor_free(vm_cursor *cursor){
	if(cursor->real_iblock != 0)
		dbindex_iblock_unlock(cursor->index, cursor->real_iblock);
	
	list_unlink(&cursor->index->cursors, cursor);
	
	free(cursor);
}
/* }}}1 */

static unsigned int dbindex_vm_search(dbindex *index, vm_cursor **cursor, char *key2_conv){
	int             cret;
	unsigned int    ret;
	vm_cursor      *range_start;
	vm_cursor      *range_end;
	vm_cursor      *current;
	char           *current_ptr;
	unsigned long   current_addr;
	unsigned long   range_elements;
	char           *key1_conv;
	
	range_start = dbindex_vm_cursor_new(index, CURSOR_READ, 0, SEEK_SET);
	range_end   = dbindex_vm_cursor_new(index, CURSOR_READ | CURSOR_FIXED, 0, SEEK_END);
	
	if(range_start->virt_addr == range_end->virt_addr){
		*cursor = range_end;
		
		dbindex_vm_cursor_prev(range_end);
		dbindex_vm_cursor_free(range_start);
		return 2;
	}
	
	current     = dbindex_vm_cursor_new(index, CURSOR_READ, 0, SEEK_END);
	//dbindex_vm_cursor_prev(range_end);
	
	while(1){
		range_elements = (range_end->virt_addr - range_start->virt_addr) / index->item_size;
		current_addr   = range_start->virt_addr + (range_elements / 2) * index->item_size;
		current_ptr    = dbindex_vm_cursor_seek(current, current_addr, SEEK_SET);
		/*
		printf("loop: elements: %llx addr: %llx ptr: %llx, range: %llx-%llx\n",
			range_elements,
			current_addr,
			current_ptr,
			range_start->virt_addr,
			range_end->virt_addr
		);*/
		if(current_ptr == NULL){
			printf("broken page\n");
			exit(255);
		}
		
		key1_conv = index->keyconv(index, current_ptr);
		
		if(key1_conv == NULL || key2_conv == NULL){
			printf("loop: elements: %llx addr: %llx ptr: %llx, range: %llx-%llx\n",
			range_elements,
			current_addr,
			current_ptr - index->file.data,
			range_start->virt_addr,
			range_end->virt_addr
		);		printf("damn data!\n");
			*cursor = NULL;
			exit(255);
			return 0;
		}
		
		cret      = index->cmpfunc(index, key1_conv, key2_conv);
		
		if(cret == 0){
			current->query = key2_conv;
			
			while(dbindex_search_slide(current, -1) == 0);
			
			*cursor = current;
			
			current->query_result = SEARCH_FOUND;
			
			dbindex_vm_cursor_free(range_start);
			dbindex_vm_cursor_free(range_end);
			
			return 0;
		}else if(cret < 0){
			dbindex_vm_cursor_seek(range_end,   current_addr, SEEK_SET);
		}else{
			dbindex_vm_cursor_seek(range_start, current_addr, SEEK_SET);
		}
		
		if(range_elements <= 1){
			*cursor = range_end;
			
			range_end->query_result = SEARCH_NOT_FOUND;
			
			dbindex_vm_cursor_prev(range_end);
			
			ret = (range_end->page_len == 0) ? 1 : 2;
			
			dbindex_vm_cursor_free(current);
			dbindex_vm_cursor_free(range_start);
			
			return ret;
		}
	}
}

/* }}}1 */

static unsigned long dbindex_expand(dbindex *index, unsigned long need_len){
	if(index->file.data_len < need_len){
		return dbmap_expand(&index->file, need_len - index->file.data_len);
	}
	return 1;
}

/* DBIndex initialisation {{{1 */
unsigned int dbindex_create(char *path, unsigned int type, unsigned int key_len, unsigned int value_len){
	unsigned long  res;
	unsigned int   ret = 1;
	unsigned short iblock;
	dbindex        new_index;
	
	if(key_len <= 2)
		return ret;
	
	memset(&new_index, 0, sizeof(dbindex));
	
	unlink(path); // SEC danger
	
	dbmap_map(path, &new_index.file);
	
	res = dbindex_expand(&new_index, DBINDEX_DATA_OFFSET);
	if(res != -1){
		new_index.type      = type;
		new_index.item_size = key_len + value_len;
		new_index.key_len   = key_len;
		new_index.value_len = value_len;
		new_index.page_size = (1 << (key_len - 2) * 8) * (key_len + value_len);
		
		int switch_ok = 0;
		switch(type){
			case DBINDEX_TYPE_PRIMARY:
			case DBINDEX_TYPE_INDEX:   // same layout as primary
				new_index.data_offset = DBINDEX_DATA_OFFSET + 0x20000 + 0x10000 / 8;
				new_index.iblock_last = 0;
				
				res = dbindex_expand(&new_index, DBINDEX_DATA_OFFSET + 0x20000 + 0x10000 / 8);
				if(res == -1)
					break;
				
				iblock = dbindex_iblock_new(&new_index);
				if(iblock == 0)
					break;
				
				// TODO check, can i use it for PRIMARY?
				if(type == DBINDEX_TYPE_INDEX){
					*(unsigned short *)(new_index.file.data + DBINDEX_DATA_OFFSET) = iblock;
				}
				
				switch_ok = 1;
				break;
		};
		if(switch_ok == 1){
			memcpy(new_index.file.data, &new_index, DBINDEX_SETTINGS_LEN);
			
			ret = 0;
		}
	}
	dbmap_unmap(&new_index.file);
	
	return ret;
}

void dbindex_load(char *path, dbindex *index){
	int      i, j;
	void    *ipage_ptr;
	short   *idata_ptr;
	short   *fdata_ptr;
	short    ipage_block;
	bool     ipage_not_null;
	unsigned short k;
	
	dbmap_map(path, &index->file);
	//pthread_rwlock_init(&index->lock, NULL);
	
	// TODO sanity check
	memcpy(index, index->file.data, DBINDEX_SETTINGS_LEN);
	
	fdata_ptr = (short *)(index->file.data + DBINDEX_DATA_OFFSET);
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
		case DBINDEX_TYPE_INDEX:

			index->iblock_locks = zalloc(0xFFFF * sizeof(pthread_rwlock_t *));
			for(k=1; k <= index->iblock_last; k++){
				dbindex_iblock_lock_init(index->iblock_locks, k);
			}
			break;
		default:
			fprintf(stderr, "%s: wrong index type\n", path); // TODO add DEBUG define
			return;
	};
	
	list_init(&index->cursors);
	
	index->loaded = 1;
}

void dbindex_save(dbindex *index){
	int      i, j;
	short   *idata_ptr;
	short   *fdata_ptr;
	void    *ipage_ptr;
	
	//pthread_rwlock_rdlock(&index->lock);
	dbmap_lock(&index->file);
		
		memcpy(index->file.data, index, DBINDEX_SETTINGS_LEN);
		
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
			case DBINDEX_TYPE_INDEX:
				break;
		};
	dbmap_unlock(&index->file);
	//pthread_rwlock_unlock(&index->lock);
}

void dbindex_unload(dbindex *index){
	int    i;
	void  *ipage_ptr;
	
	if(index->loaded != 1)
		return;

	dbindex_save(index);

	//pthread_rwlock_wrlock(&index->lock);
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
			case DBINDEX_TYPE_INDEX:
				for(i=1; i <= index->iblock_last; i++){
					dbindex_iblock_lock_free(index->iblock_locks, i);
				}
				free(index->iblock_locks);
				break;
		};
		dbmap_unmap(&index->file);
	//pthread_rwlock_unlock(&index->lock);
	index->loaded = 0;
}

void dbindex_set_funcs(dbindex *index, int (*cmpfunc)(dbindex *, void *, void *), void *(*keyconv)(dbindex *, void *)){
	index->cmpfunc = cmpfunc;
	index->keyconv = keyconv;
}

void dbindex_set_userdata(dbindex *index, void *data){
	index->user_data = data;
}

/* }}}1 */
/* DBIndex IPages {{{1 */
static short dbindex_ipage_get_iblock(dbindex* index, void *item_key){
	unsigned char    index_c1;
	unsigned char    index_c2;
	short           *fdata_ptr;

	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			fdata_ptr = (short *)(index->file.data + DBINDEX_DATA_OFFSET);
			
			index_c1 = *((char *)item_key + 0x03);
			index_c2 = *((char *)item_key + 0x02);

			return *(fdata_ptr + index_c1 * 256 + index_c2);
			break;
		case DBINDEX_TYPE_INDEX:
			// bsearch block
			break;
	}
}

static void dbindex_ipage_set_iblock(dbindex* index, void *item_key, unsigned short iblock){
	unsigned char    index_c1;
	unsigned char    index_c2;
	short           *fdata_ptr;
	
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			fdata_ptr = (short *)(index->file.data + DBINDEX_DATA_OFFSET);
			
			index_c1 = *((char *)item_key + 0x03);
			index_c2 = *((char *)item_key + 0x02);
			
			*(fdata_ptr + index_c1 * 256 + index_c2) = iblock;
			// msync
			break;
		case DBINDEX_TYPE_INDEX:
			// nothing to do
			break;
	};
}
/* }}}1 */

unsigned int dbindex_insert(dbindex *index, void *item_key, void *item_value){
	if(index->loaded == 0)
		return 1;
	
	unsigned short iblock;
	unsigned short iblock_new;	
	unsigned short iblock_curr_virt;
	unsigned short iblock_curr_real;
	unsigned short iblock_new_virt;
	unsigned short iblock_new_real;
	unsigned short iblock_prev_virt;
	unsigned short iblock_prev_real;
	
	unsigned int   ret = 1;
	unsigned int   cret;
	char          *item_offset;
	char          *new_key       = zalloc(index->item_size);
	char          *key_conv;
	vm_cursor     *cursor;

	//pthread_rwlock_rdlock(&index->lock);
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			// if item have mpage - we write it to this page
			// if item dont have own page - we write it to last page
						
			/* prepare item */
			revmemcpy(new_key, item_key, index->key_len);
			memcpy(new_key + index->key_len, item_value, index->value_len);	
			
			
			iblock = dbindex_ipage_get_iblock(index, item_key);
			if(iblock == 0)
				iblock = index->iblock_last;
			
			dbmap_lock(&index->file);
			dbindex_iblock_wrlock(index, iblock);
				
				cret = dbindex_iblock_search(index, iblock, &item_offset, new_key);
				cret = dbindex_iblock_insert(index, iblock, cret, item_offset, new_key);
				if(cret == 1){
					dbmap_unlock(&index->file);
					
					iblock_new = dbindex_iblock_new(index);
					
					if(iblock_new == 0){
						// no more blocks
						dbindex_iblock_unlock(index, iblock);
						break;
					}
					
					dbmap_lock(&index->file);
					dbindex_iblock_wrlock(index, iblock_new);
						
						char  *elements_start;
						char  *elements_end;
						
						ret = dbindex_iblock_get_ourelements(index, iblock, new_key, &elements_start, &elements_end);
						if(ret == 0){
							dbindex_iblock_move_elements(index, iblock, iblock_new, elements_start, elements_end);
						}
						cret = dbindex_iblock_search(index, iblock_new, &item_offset, new_key);
						cret = dbindex_iblock_insert(index, iblock_new, cret, item_offset, new_key);
						
					dbindex_iblock_unlock(index, iblock_new);	
					dbindex_iblock_unlock(index, iblock);
					dbmap_unlock(&index->file);
				}else{
					dbindex_iblock_unlock(index, iblock);
					dbmap_unlock(&index->file);
					
					iblock_new = iblock;
				}
			
			dbindex_ipage_set_iblock(index, item_key, iblock_new);
			
			ret = 0;
			break;
		case DBINDEX_TYPE_INDEX:
			/* prepare item */
			key_conv = index->keyconv(index, item_key);	
			
			/* insert item */
			ret = dbindex_vm_search      (index, &cursor, key_conv);
			ret = dbindex_vm_item_insert (cursor, item_key);		
			
			dbindex_vm_cursor_free(cursor);
			break;
	};
	//pthread_rwlock_unlock(&index->lock);
	free(new_key);

	return ret;
}


/* dbindex_query
 *
 * INDEX:  in: item_value - converted key
 *        out: item_key   - key
 * returns: 
 *     0 - found
 *     1 - not found
*/

unsigned int dbindex_query(dbindex *index, void *item_key, void *item_value){
	unsigned int     ret = 1;
	unsigned int     cret;
	unsigned short   iblock_real;
	unsigned short   iblock_virt;
	unsigned char    index_c1;
	unsigned char    index_c2;
	char            *item_revkey = malloc(index->key_len);
	char            *item_offset;
	unsigned short  *ipage_l2;

	//pthread_rwlock_rdlock(&index->lock);
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
				iblock_real = dbindex_ipage_get_iblock(index, item_key);
				if(iblock_real == 0)
					break;
				
				dbmap_lock(&index->file);
				dbindex_iblock_rdlock(index, iblock_real);
					
					revmemcpy(item_revkey, item_key, index->key_len);
					if(dbindex_iblock_search(index, iblock_real, &item_offset, item_revkey) == 0){
						memcpy(item_value, item_offset + index->key_len, index->value_len);
						ret = 0;
					}
					
				dbindex_iblock_unlock(index, iblock_real);
				dbmap_unlock(&index->file);
				break;
		}
	//pthread_rwlock_unlock(&index->lock);
	
	free(item_revkey);
	return ret;
}

vm_cursor* dbindex_search(dbindex *index, void *query){
	unsigned int     cret;
	vm_cursor       *cursor;
	
	switch(index->type){
		case DBINDEX_TYPE_INDEX:
			dbmap_lock(&index->file);
				
				cret = dbindex_vm_search(index, &cursor, query);
				if(cret != SEARCH_FOUND){
					dbindex_vm_cursor_free(cursor);
					cursor = NULL;
				}
				
			dbmap_unlock(&index->file);	
			break;
	}
	return cursor;
}

unsigned int dbindex_search_slide(vm_cursor *cursor, int direction){
	unsigned int  cret;
	char         *key1_conv;
	char         *real_ptr;
	
	if(direction < 0)
		real_ptr = cursor->real_ptr - cursor->index->item_size;
	else
		real_ptr = cursor->real_ptr + cursor->index->item_size;
	
	// check diapasone of real_ptr

	if((key1_conv = cursor->index->keyconv(cursor->index, real_ptr)) == NULL)
		return 1;
	
	if((cret = cursor->index->cmpfunc(cursor->index, key1_conv, cursor->query)) != 0)
		return 1;
	
	if(direction < 0){
		if(dbindex_vm_cursor_prev(cursor) == NULL)
			return 1;
	}else{
		if(dbindex_vm_cursor_next(cursor) == NULL)
			return 1;
	}
	return 0;	
}

/* dbindex_delete
 *
 * returns:
 * 	0 - found and deleted
 * 	1 - not found
*/

// TODO move actual deletion to another thread, mark item as deleted

unsigned int dbindex_delete(dbindex *index, void *item_key){
	short            iblock;
	unsigned int     ret = 1;
	char            *item_offset;
	char            *key_conv;
	vm_cursor       *cursor;
	
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			iblock = dbindex_ipage_get_iblock(index, item_key);
			if(iblock == 0)
				break;
			
			dbmap_lock(&index->file);
			dbindex_iblock_wrlock(index, iblock);
				
				char  *item_revkey = malloc(index->key_len);
				revmemcpy(item_revkey, item_key, index->key_len);
				if(dbindex_iblock_search(index, iblock, &item_offset, item_revkey) == 0){
					dbindex_iblock_delete(index, iblock, item_offset);
					
					ret = 0;
				}
				free(item_revkey);
				
			dbindex_iblock_unlock(index, iblock);
			dbmap_unlock(&index->file);
			break;
		case DBINDEX_TYPE_INDEX:
			key_conv = index->keyconv(index, item_key);	
			
			if( (cursor = dbindex_search(index, key_conv)) != NULL){
				do {
					if(memcmp(item_key, cursor->real_ptr, index->key_len) == 0){
						ret = dbindex_vm_item_delete(cursor);
						break;
					}
				}while(dbindex_search_slide(cursor, 1) == 0);
				
				dbindex_vm_cursor_free(cursor);
			}
			break;
	};
	
	return ret;
}

unsigned int dbindex_iter(dbindex *index, void *(*func)(void *, void *, void *, void *), void *arg1, void *arg2){
	unsigned short iblock;
	unsigned int   ret;
	unsigned int   stop = 0;
	char          *item_offset;
	char          *page_data_start;
	char          *page_data_end;
	char          *new_key = malloc(index->item_size);
	
	dbmap_lock(&index->file);
	
	iblock = 1;
	while(iblock <= index->iblock_last && stop == 0){
		dbindex_iblock_rdlock(index, iblock);
			
			ret = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
			
			item_offset = page_data_start;
			while(item_offset < page_data_end){
				revmemcpy(new_key, item_offset, index->key_len);
				
				if(func(new_key, (item_offset + index->key_len), arg1, arg2) == NULL){
					stop = 1;
					break;
				}
				
				item_offset += index->item_size;
			}
		dbindex_iblock_unlock(index, iblock);
		iblock++;
	}
	
	dbmap_unlock(&index->file);
	
	free(new_key);
}

