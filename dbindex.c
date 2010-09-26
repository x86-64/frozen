// 2 level indexing:
//  - page size = 12 * 2^16
//  - elements in one page - 2^16

// NOTE indexes written to support 4 bytes key, to use 8 bytes keys i'll
//      write balancer with transparent redirection

#define DBINDEX_TYPE_PRIMARY 0

#define DBINDEX_DATA_OFFSET  0x40
#define DBINDEX_SETTINGS_LEN 0x1E

typedef struct dbindex_ dbindex;
struct dbindex_ {
	/* settings, same as in index file */
	unsigned int        type;                // 4 bytes
	unsigned int        item_size;           // 4 bytes
	unsigned long       page_size;           // 8 bytes
	unsigned int        key_len;             // 4 bytes
	unsigned int        value_len;           // 4 bytes
	unsigned int        data_offset;         // 4 bytes
	unsigned short      iblock_last;         // 2 bytes
	
	/* specific variables */
	void              **ipage_l1;            // main page of PRIMARY index
	pthread_rwlock_t  **iblock_locks;
	
	dbmap               file;
	//pthread_rwlock_t    lock;
};

/*
 * Index file layout:
	
	0x00000000: 0x40: index settings
	
	PRIMARY INDEX:
	0x00000040: 0x20000: ipage's 256 pointers 2 byte each = 0x100 * 2 = 0x200 * 0x100 = 0x20000
	0x00020040: undef  : data
*/

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
	pthread_rwlock_wrlock( index->iblock_locks[iblock] );
}

static void dbindex_iblock_rdlock(dbindex* index, unsigned short iblock){
	pthread_rwlock_rdlock( index->iblock_locks[iblock] );
}

static void dbindex_iblock_unlock(dbindex* index, unsigned short iblock){
	pthread_rwlock_unlock( index->iblock_locks[iblock] );
}

static char *dbindex_iblock_get_page(dbindex *index, unsigned short iblock){
	return index->file.data + index->data_offset + (iblock - 1) * index->page_size;
}

/* iblock start {{{1 */
static void  dbindex_iblock_init_start(dbindex *index, unsigned short iblock, unsigned int value){
	char *page_offset = dbindex_iblock_get_page(index, iblock);
	
	memset(page_offset, 0xFF, index->item_size);
	*((unsigned int *)page_offset) = value;
}

/* dbindex_iblock_get_start
 *
 * returns space left in page
 * 	*page_data_start - ptr to data start
 * 	*page_data_end   - ptr to data end
*/

static unsigned int dbindex_iblock_get_start(dbindex *index, unsigned short iblock, char **page_data_start, char **page_data_end){
	unsigned int  i;
	unsigned int  free_size;
	char         *page_offset = dbindex_iblock_get_page(index, iblock); 
	
	*page_data_end = page_offset + index->page_size;

	for(i=0; i<index->item_size - 4; i++){
		if(page_offset[i+4] != (char)0xFF){
			*page_data_start = page_offset;
			return 0; // page full
		}
	}
	free_size = *((unsigned int *)page_offset);

	*page_data_start = page_offset + free_size;
	return free_size;
}

static void  dbindex_iblock_set_start(dbindex *index, unsigned short iblock, unsigned int delta){
	unsigned int  ret;
	char         *page_data_start;
	char         *page_data_end;
	
	ret = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
	if(ret == 0)
		return;
	
	*((unsigned int *)(page_data_start - ret)) -= delta;
}
/* }}}1 */

/* dbindex_iblock_search
 *
 * returns:
 *     0 - item pointed by *new_key found at pitem_offset address
 *     1 - item not found, pitem_offset points to address there this item can be, page is full
 *     2 - item not found, pitem_offset points to address there this item can be, page is not full
*/

static int   dbindex_iblock_search(dbindex *index, unsigned short iblock, char **pitem_offset, char *new_key){
	// TODO remake with binary search
	unsigned int  ret;
	char         *item_offset;
	char         *page_data_start;
	char         *page_data_end;

	ret = dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);

	/* full scan
	item_offset = page_data_start;
	while(item_offset < page_data_end){
		int ret = memcmp(item_offset, new_key, index->key_len);
		if(ret == 0){
			*pitem_offset = item_offset;
			return 0;
		}
		if(ret < 0){
			break;
		}
		item_offset += index->item_size;
	}
	*/
	
	/* binary search */
	char          *range_start = page_data_start;
	char          *range_end   = page_data_end;
	unsigned long  range_elements;
	int            cret;

	while(1){
		range_elements = (range_end - range_start) / index->item_size;
		
		if(range_elements == 0)
			break;

		item_offset = range_start + (range_elements / 2) * index->item_size;
		
		/*printf("bsearch loop %ld: %lld elements %llx in %llx-%llx\n",
			iblock,
			range_elements,
			item_offset - page_data_start,
			range_start - page_data_start,
			range_end   - page_data_start
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
	}
	
	*pitem_offset = item_offset - index->item_size;
	
	return (ret == 0) ? 1 : 2;
}

static short dbindex_iblock_new(dbindex *index){
	short  iblock;
	char  *iblock_offset;
	
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			
			dbmap_expand(&index->file, index->page_size);
			iblock = ++index->iblock_last;
			
			dbindex_iblock_init_start (index, iblock, index->page_size);
			dbindex_iblock_lock_init  (index->iblock_locks, iblock);
			break;
	};

	return iblock;
}

static void revmemcpy(char *dst, char *src, unsigned long len){
	src += len - 1;
	while(len){
		*dst++ = *src--;
		len--;
	}
}

static short dbindex_iblock_insert(dbindex *index, unsigned short iblock, void *item_key, void *item_value){
	// if have free space - write
	// if dont have free space - get all our elements, alloc new page and write it
	
	short          iblock_final = iblock;
	int            i,j,ret;
	char          *item_offset;
	char          *new_key = zalloc(index->item_size);
		
	//for(j=0, i=index->key_len - 1; i >= 0; i--, j++)
	//	new_key[j] = ((char *)item_key)[i];
	revmemcpy(new_key, item_key, index->key_len);
	memcpy(new_key + index->key_len, item_value, index->value_len);	

	bool item_written = false;

	dbmap_lock(&index->file);
		dbindex_iblock_wrlock(index, iblock);

			ret = dbindex_iblock_search(index, iblock, &item_offset, new_key);
			if(ret == 0){
				// found existing item
				memcpy(item_offset, new_key, index->item_size);
				item_written = true;
			}else if(ret == 2){
				// insert ne
				char *page_data_start;
				char *page_data_end;
				char *page_data_start_new;
	
				dbindex_iblock_get_start(index, iblock, &page_data_start, &page_data_end);
	
				page_data_start_new = page_data_start - index->item_size;
	
				if(item_offset >= page_data_start)
					memcpy(page_data_start_new, page_data_start, item_offset - page_data_start + index->item_size);
				
				memcpy(item_offset, new_key, index->item_size);
				
				dbindex_iblock_set_start(index, iblock, index->item_size);
				item_written = true;
			}
			
		dbindex_iblock_unlock(index, iblock);
	dbmap_unlock(&index->file);

	if(item_written == false){
		short  iblock_new = dbindex_iblock_new(index);
		
		dbmap_lock(&index->file);
		dbindex_iblock_wrlock(index, iblock_new);
		dbindex_iblock_rdlock(index, iblock);

			dbindex_iblock_search(index, iblock, &item_offset, new_key); // rerun search, map can be moved
		
			char  *our_elements_start;
			char  *our_elements_end;
			char  *our_element         = zalloc(index->item_size);
				
			our_element[0] = new_key[0];
			our_element[1] = new_key[1];
			dbindex_iblock_search(index, iblock, &our_elements_end, our_element);
			our_element[2] = (char)0xFF;
			our_element[3] = (char)0xFF;
			dbindex_iblock_search(index, iblock, &our_elements_start, our_element);
			our_elements_start += index->item_size;
			our_elements_end   += index->item_size;
			item_offset        += index->item_size;
					
			free(our_element);
	
			unsigned int  iblock_free_len;
			char         *iblock_data_start;
			char         *iblock_data_end;
			char         *iblock_new_data_start;
			char         *iblock_new_data_end;
		
			iblock_free_len = dbindex_iblock_get_start(index, iblock, &iblock_data_start, &iblock_data_end);
					  dbindex_iblock_get_start(index, iblock_new, &iblock_new_data_start, &iblock_new_data_end);

			unsigned long  new_items_p2_len = (our_elements_end - item_offset);
			unsigned long  new_items_p1_len = (item_offset - our_elements_start);
			unsigned long  new_items_len    = new_items_p1_len + new_items_p2_len;
			char          *new_items_p2_pos = iblock_new_data_end - new_items_p2_len;
			char          *new_items_i_pos  = new_items_p2_pos    - index->item_size;
			char          *new_items_p1_pos = new_items_i_pos     - new_items_p1_len;
			
			
			/* copy our elements to new page */
			if(new_items_p1_len > 0)
				memcpy(new_items_p1_pos, our_elements_start, new_items_p1_len);
			memcpy(new_items_i_pos, new_key, index->item_size);
			if(new_items_p2_len > 0)
				memcpy(new_items_p2_pos, item_offset, new_items_p2_len);
			
			/* set correct page length */
			dbindex_iblock_set_start(index, iblock_new, new_items_len + index->item_size);
		
			/* set corrent iblock */
			iblock_final = iblock_new;
			
		dbindex_iblock_unlock(index, iblock);
			
			/* correct old iblock */
			if(new_items_len > 0){
				dbindex_iblock_wrlock(index, iblock);
					
					memmove(iblock_data_start + new_items_len, iblock_data_start, our_elements_start - iblock_data_start);
					memset(iblock_data_start, 0, new_items_len); // NOTE can remove
					
					dbindex_iblock_init_start(index, iblock, iblock_free_len + new_items_len);
				
				dbindex_iblock_unlock(index, iblock);
			}
		
		dbindex_iblock_unlock(index, iblock_new);
		dbmap_unlock(&index->file);
	}
	
	free(new_key);
	return iblock_final;
}

static void dbindex_expand(dbindex *index, unsigned long need_len){
	if(index->file.data_len < need_len){
		dbmap_expand(&index->file, need_len - index->file.data_len);
	}
}

void dbindex_create(char *path, unsigned int type, unsigned int key_len, unsigned int value_len){
	dbindex new_index;
		
	if(key_len <= 2)
		return;
	
	memset(&new_index, 0, sizeof(dbindex));

	unlink(path); // SEC dangerous
	
	dbmap_map(path, &new_index.file);
	dbindex_expand(&new_index, DBINDEX_DATA_OFFSET);
	
	new_index.type      = type;
	new_index.item_size = key_len + value_len;
	new_index.key_len   = key_len;
	new_index.value_len = value_len;
	new_index.page_size = (1 << (key_len - 2) * 8) * (key_len + value_len);

	switch(type){
		case DBINDEX_TYPE_PRIMARY:
			new_index.data_offset = DBINDEX_DATA_OFFSET + 0x20000;
			new_index.iblock_last = 0;

			dbindex_expand(&new_index, DBINDEX_DATA_OFFSET + 0x20000);
			dbindex_iblock_new(&new_index);
			break;
	};
	memcpy(new_index.file.data, &new_index, DBINDEX_SETTINGS_LEN);
	
	dbmap_unmap(&new_index.file);
}

void dbindex_load(char *path, dbindex *index){
	int      i, j;
	void    *ipage_ptr;
	short   *idata_ptr;
	short   *fdata_ptr;
	short    ipage_block;
	bool     ipage_not_null;

	dbmap_map(path, &index->file);
	//pthread_rwlock_init(&index->lock, NULL);

	// TODO sanity check
	memcpy(index, index->file.data, DBINDEX_SETTINGS_LEN);

	fdata_ptr = (short *)(index->file.data + DBINDEX_DATA_OFFSET);
	switch(index->type){
		case DBINDEX_TYPE_PRIMARY:
			// TODO sanity check
			index->ipage_l1 = (void **)malloc(256 * sizeof(void *));
			if(index->ipage_l1 == NULL){
				fprintf(stderr, "dbindex_load: insufficient memory\n");
				return;
			}
			for(i=0; i<=255; i++){
				ipage_ptr = (void *)malloc(256 * sizeof(short));
				if(ipage_ptr == NULL){
					fprintf(stderr, "dbindex_load: insufficient memory\n");
					return;
				}

				idata_ptr = ipage_ptr;
				ipage_not_null = false;
				for(j=0; j<=255; j++){
					ipage_block = *fdata_ptr++;
					if(ipage_block != 0)
						ipage_not_null = true;
					*idata_ptr++ = ipage_block;
				}
				if(ipage_not_null == false){
					free(ipage_ptr);
					ipage_ptr = NULL;
				}
				index->ipage_l1[i] = ipage_ptr;
			}
			
			unsigned short i;

			index->iblock_locks = zalloc(0xFFFF * sizeof(pthread_rwlock_t *));
			for(i=1; i <= index->iblock_last; i++){
				dbindex_iblock_lock_init(index->iblock_locks, i);
			}
			break;
		default:
			fprintf(stderr, "%s: wrong index type\n", path); // TODO add DEBUG define
			return;
	};
}

void dbindex_save(dbindex *index){
	int      i, j;
	short   *idata_ptr;
	short   *fdata_ptr;
	void    *ipage_ptr;
	
	//pthread_rwlock_rdlock(&index->lock);
	dbmap_lock(&index->file);

		memcpy(index->file.data, index, DBINDEX_SETTINGS_LEN);
		
		fdata_ptr = (short *)(index->file.data + DBINDEX_DATA_OFFSET);
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
				for(i=0; i<=255; i++){
					idata_ptr = index->ipage_l1[i];
					if(idata_ptr == NULL){
						fdata_ptr += 255 * sizeof(short);
					}else{
						for(j=0; j<=255; j++)
							*fdata_ptr++ = *idata_ptr++;
					}
				}
				break;
	
		};
	dbmap_unlock(&index->file);
	//pthread_rwlock_unlock(&index->lock);
}

void dbindex_unload(dbindex *index){
	int    i;
	void  *ipage_ptr;

	dbindex_save(index);

	//pthread_rwlock_wrlock(&index->lock);
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
				if(index->ipage_l1 != NULL){
					for(i=0; i<=255; i++){
						ipage_ptr = index->ipage_l1[i];
						if(ipage_ptr != NULL)
							free(ipage_ptr);
					}
					free(index->ipage_l1);
				}
				
				for(i=1; i <= index->iblock_last; i++){
					dbindex_iblock_lock_free(index->iblock_locks, i);
				}
				free(index->iblock_locks);
				break;
		};
		dbmap_unmap(&index->file);
	//pthread_rwlock_unlock(&index->lock);
}

short dbindex_ipage_get_iblock(dbindex* index, void *item_key){
	unsigned char    index_c1;
	unsigned char    index_c2;
	short           *ipage_l2;

	index_c1 = *((char *)item_key + 0x03);
	index_c2 = *((char *)item_key + 0x02);
				
	if(index->ipage_l1 == NULL)
		return 0;

	ipage_l2 = (short *)index->ipage_l1[index_c1];
	if( ipage_l2 == NULL )
		return 0;
	
	return ipage_l2[index_c2];
}

void dbindex_ipage_set_iblock(dbindex* index, void *item_key, unsigned short iblock){
	unsigned char    index_c1;
	unsigned char    index_c2;
	short           *ipage_l2;

	index_c1 = *((char *)item_key + 0x03);
	index_c2 = *((char *)item_key + 0x02);
	
	ipage_l2 = (short *)index->ipage_l1[index_c1];
	if( ipage_l2 == NULL ){
		ipage_l2 = (short *)zalloc(256 * sizeof(short));
		if(ipage_l2 == NULL){
			// NOTE holly shit..
			fprintf(stderr, "dbindex_insert: insufficient memory, losing data\n");
			return;
		}
		index->ipage_l1[index_c1] = ipage_l2;
	}
	ipage_l2[index_c2] = iblock;
}

void dbindex_insert(dbindex *index, void *item_key, void *item_value){
	unsigned short iblock;
	unsigned short iblock_new;

	//pthread_rwlock_rdlock(&index->lock);
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
				// if item have mpage - we write it to this page
				// if item dont have own page - we write it to last page
				
				iblock = dbindex_ipage_get_iblock(index, item_key);
				if(iblock == 0)
					iblock = index->iblock_last;
				
				iblock_new = dbindex_iblock_insert(index, iblock, item_key, item_value);
				
				dbindex_ipage_set_iblock(index, item_key, iblock_new);
			break;
		};
	//pthread_rwlock_unlock(&index->lock);
}


/* dbindex_query
 *
 * returns: 
 *     0 - found
 *     1 - not found
*/

unsigned int dbindex_query(dbindex *index, void *item_key, void *item_value){
	short            iblock;
	unsigned int     ret = 1;
	unsigned char    index_c1;
	unsigned char    index_c2;
	char            *item_revkey = malloc(index->key_len);
	char            *item_offset;
	short           *ipage_l2;

	//pthread_rwlock_rdlock(&index->lock);
		switch(index->type){
			case DBINDEX_TYPE_PRIMARY:
				iblock = dbindex_ipage_get_iblock(index, item_key);
				if(iblock == 0)
					break;
				
				dbmap_lock(&index->file);
				dbindex_iblock_rdlock(index, iblock);
					
					revmemcpy(item_revkey, item_key, index->key_len);
					if(dbindex_iblock_search(index, iblock, &item_offset, item_revkey) == 0){
						memcpy(item_value, item_offset + index->key_len, index->value_len);
						ret = 0;
					}			
					
				dbindex_iblock_unlock(index, iblock);
				dbmap_unlock(&index->file);
				break;
		}
	//pthread_rwlock_unlock(&index->lock);
	return ret;
}

