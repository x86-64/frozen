// 2 level indexing:
//  - page size = 12 * 2^16
//  - elements in one page - 2^16

// NOTE indexes written to support 4 bytes key, to use 8 bytes keys i'll
//      write balancer with transparent redirection

#define DBINDEX_TYPE_PRIMARY 0
#define DBINDEX_TYPE_INDEX   1

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
	
	list                cursors;
	dbmap               file;
	int                 loaded;
	//pthread_rwlock_t    lock;
	void               *user_data;
	
	int    (*cmpfunc)(dbindex *, void *, void *); // cmp two converted keys
	void * (*keyconv)(dbindex *, void *);         // convert key function
};


#define CURSOR_READ       1
#define CURSOR_WRITE      2
#define CURSOR_FIXED      4

typedef struct vm_cursor_ vm_cursor;
struct vm_cursor_ {
	unsigned long   page_len;
	int             rw;
	dbindex        *index;
	char           *base_addr;
	
	unsigned long   virt_addr;
	unsigned short  virt_iblock;
	unsigned long   real_addr;
	unsigned short  real_iblock;
	char           *real_ptr;
	
	void           *query;
	int             query_result;
};


/*
 * Index file layout:
	
	0x00000000: 0x40: index settings
	
	PRIMARY INDEX:
	0x00000040: 0x20000: ipage's 256 pointers 2 byte each = 0x100 * 2 = 0x200 * 0x100 = 0x20000
	0x00020040: undef  : data
*/

unsigned int dbindex_create(char *path, unsigned int type, unsigned int key_len, unsigned int value_len);
void dbindex_load(char *path, dbindex *index);
void dbindex_save(dbindex *index);
void dbindex_unload(dbindex *index);
void dbindex_set_funcs(dbindex *index, int (*cmpfunc)(dbindex *, void *, void *), void *(*keyconv)(dbindex *, void *));
unsigned int dbindex_insert(dbindex *index, void *item_key, void *item_value);
unsigned int dbindex_query(dbindex *index, void *item_key, void *item_value);
unsigned int dbindex_delete(dbindex *index, void *item_key);
unsigned int dbindex_iter(dbindex *index, void *(*func)(void *, void *, void *, void *), void *arg1, void *arg2);
vm_cursor*   dbindex_search(dbindex *index, void *query);
unsigned int dbindex_search_slide(vm_cursor *cursor, int direction);

char* dbindex_vm_cursor_seek(vm_cursor *c, unsigned long long value, int whence);
char* dbindex_vm_cursor_next(vm_cursor *cursor);
char *dbindex_vm_cursor_prev(vm_cursor *cursor);
vm_cursor* dbindex_vm_cursor_new(dbindex *index, int type, unsigned long value, int whence);
void dbindex_vm_cursor_free(vm_cursor *cursor);
static void* dbindex_iter_cursors_recalc(void *cursor, void *null1, void *null2);
	
