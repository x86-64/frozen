#include <memcachedb.h>
#include <list.h>
#include <dbmap.h>
#include <dbindex.h>
#include <db.h>
#include <data.h>

static list             db_tables;
static unsigned int     db_destroy_ok = 0;
static unsigned long    db_oid_last;
static pthread_mutex_t  db_oid_last_mutex;
static pthread_mutex_t  db_mutex_query_init;

/* Callbacks proto */

static void* db_iter_table_search(void *table, void *name, void *addr);
static void* db_iter_query_delete_oid(void *table, void *oid, void *null);
static int   db_cmpfunc_idx_data(dbindex *index, void *item1_key, void *item2_key);
static void* db_keyconv_idx_data(dbindex *index, void *item_key);

/* PRIVATE FUNCTIONS */

/* Filesystem functions {{{1 */
static char *db_abspath(char *name, char *ext){
	char   fullpath[PATH_MAX];
	size_t name_len = strlen(name);
	size_t path_len = strlen(settings.dbpath);
	size_t ext_len  = strlen(ext);

	if(path_len + name_len + 1 + ext_len + 1 >= PATH_MAX) return NULL;
	
	memcpy(&fullpath,                      settings.dbpath, path_len);
	memcpy(&fullpath[path_len],            name,            name_len);
	memcpy(&fullpath[path_len+name_len+1], ext,             ext_len + 1);
	fullpath[path_len+name_len] = '.';

	return strdup(fullpath);
}

/* }}}1 */
/* DBTable {{{1 */
/* DBTable settings {{{2 */
static void db_table_settings_save(dbtable *table){
	char *apath_set  = db_abspath(table->name, "set");

	FILE *fp_set = fopen(apath_set, "w");
	
	fprintf(fp_set, "%d\n", table->data_type);
	
	fclose(fp_set);
}

static void db_table_settings_load(dbtable *table){
	int          res;
	struct stat  file_stat;

	char *apath_set  = db_abspath(table->name, "set");
	if(stat(apath_set, &file_stat) != 0){
		table->data_type = 5;
		
		db_table_settings_save(table);
	}else{
		FILE *fp_set = fopen(apath_set, "r");

		res = fscanf(fp_set, "%d\n", &table->data_type);

		fclose(fp_set);
	}
}
/* }}}2 */

static void db_table_load_idx_data(dbtable *table){	
	struct stat index_stat;

	char *apath_idx_data = db_abspath(table->name, "idx1");
	if(stat(apath_idx_data, &index_stat) == 0){
		dbindex_load(apath_idx_data, &table->idx_data);
		dbindex_set_userdata (&table->idx_data, table);
		dbindex_set_funcs    (&table->idx_data, &db_cmpfunc_idx_data, &db_keyconv_idx_data);
	}
	free(apath_idx_data);
}

static dbtable* db_table_load(char *name){
	unsigned int res;
	dbtable     *table = NULL;
	struct stat  index_stat;
	
	if(settings.verbose > 1)
		printf("loading table '%s'\n", name);

	// is already loaded?
	list_iter(&db_tables, &db_iter_table_search, name, &table);
	if(table != NULL)
		return;
	
	table = zalloc(sizeof(dbtable));
	table->name = strdup(name);
	
	db_table_settings_load(table);

	// load data
	char *apath_data    = db_abspath(name, "dat");
	
	if(dbmap_map(apath_data, &table->data) == 0){
		printf("failed to load '%s'\n", name);
		exit(EXIT_FAILURE);
	}
	free(apath_data);
	
	// load indexes
	char *apath_idx_oid = db_abspath(name, "idx0"); 
	
	if(stat(apath_idx_oid, &index_stat) != 0){
		res = dbindex_create(apath_idx_oid, DBINDEX_TYPE_PRIMARY, 4, 8);
		if(res == 1){
			// TODO error handling
		}
	}
	
	dbindex_load(apath_idx_oid, &table->idx_oid);
	free(apath_idx_oid);
	
	db_table_load_idx_data(table);
	
	list_push(&db_tables, table);
	
	return table;
}

static void db_table_unload(dbtable *table){
	db_table_settings_save(table);

	dbindex_unload(&table->idx_oid);
	dbindex_unload(&table->idx_data);
	dbmap_unmap(&table->data);

	free(table->name);
	free(table);
}

static void db_table_unload_by_name(char *name){
	dbtable *table = NULL;
	
	list_iter(&db_tables, &db_iter_table_search, (void *)name, (void *)&table);
	if(table == NULL)
		return;
	
	list_unlink(&db_tables, table); // unlink table to prevent access to it
	db_table_unload(table);
}

static void db_table_unlink(char *name){
	char *name_cpy = strdup(name);
	db_table_unload_by_name(name_cpy);
	
	char *file;

	file = db_abspath(name_cpy, "dat");  unlink(file); free(file);
	file = db_abspath(name_cpy, "set");  unlink(file); free(file);
	file = db_abspath(name_cpy, "idx0"); unlink(file); free(file);
	file = db_abspath(name_cpy, "idx1"); unlink(file); free(file);
	free(name_cpy);
}

static void db_table_rename(char *table_name_old, char *table_name_new){
	char *name_old = strdup(table_name_old);
	char *name_new = strdup(table_name_new);

	db_table_unload_by_name (name_old);
	db_table_unlink         (name_new);
	
	int ret;
	char *filef, *filet;
	
	filef = db_abspath(name_old, "set");  filet = db_abspath(name_new, "set");  ret = link(filef, filet); free(filef); free(filet);
	filef = db_abspath(name_old, "dat");  filet = db_abspath(name_new, "dat");  ret = link(filef, filet); free(filef); free(filet);
	filef = db_abspath(name_old, "idx0"); filet = db_abspath(name_new, "idx0"); ret = link(filef, filet); free(filef); free(filet);
	filef = db_abspath(name_old, "idx1"); filet = db_abspath(name_new, "idx1"); ret = link(filef, filet); free(filef); free(filet);
	
	db_table_unlink(name_old);
	db_table_load(name_new);

	free(name_old);
	free(name_new);
}

/* DBTable queries {{{2 */
static unsigned int db_table_query_isset(dbitem *item){
	unsigned long entry_off;
	
	return dbindex_query(&item->table->idx_oid, &item->oid, &entry_off);
}

static unsigned int db_table_query_set(dbitem *item){
	dbtable       *table      = item->table;
	char          *entry_ptr;
	unsigned long  entry_off;
	unsigned int   entry_len;
	
	entry_len = data_packed_len(table->data_type, item->data, item->data_len);
	
	// TODO search free space failed
	entry_off = dbmap_expand(&table->data, entry_len);
	
	if(entry_off == -1)
		return 1; // error
	
	dbmap_lock(&table->data);
		
		entry_ptr = table->data.data + entry_off;
		
		data_pack(table->data_type, item->data, entry_ptr, entry_len);
		
	dbmap_unlock(&table->data);
	
	dbindex_insert(&table->idx_oid,  &item->oid, &entry_off);
	dbindex_insert(&table->idx_data, &item->oid, NULL);
	
	return 0;
}

static unsigned int db_table_query_get(dbitem *item){
	unsigned long  entry_off;
	unsigned long  entry_len;
	unsigned char *entry_ptr;

	dbtable *table = item->table;

	if(dbindex_query(&table->idx_oid, &item->oid, &entry_off) == 0){
		dbmap_lock(&table->data);
			// TODO: remove copying item value to memory
			
			entry_ptr = table->data.data + entry_off;
			entry_len = data_unpacked_len(table->data_type, entry_ptr, 0);
			
			item->data     = malloc(entry_len);	
			item->data_len = entry_len;
		
			data_unpack(table->data_type, entry_ptr, entry_len, item->data);

		dbmap_unlock(&table->data);
		return 0;
	}
	return 1;
}

static unsigned int db_table_query_delete(dbitem *item){
	unsigned long  entry_off;
	unsigned int   ret;
	
	if(dbindex_query(&item->table->idx_oid, &item->oid, &entry_off) == 0){
		// TODO delete data from data file
		
		if(item->table->idx_data.loaded == 1)
			ret = dbindex_delete(&item->table->idx_data, &item->oid);
		
		ret = dbindex_delete(&item->table->idx_oid,  &item->oid);
		
		return ret;
	}
	return 1;
}
/* }}}2 */
/* }}}1 */
/* Tables {{{1 */
static void db_tables_load(void){
	DIR            *dir;
	struct dirent  *dit;
	
	dir = opendir(settings.dbpath);
	if(dir == NULL)
		return;

	while(dit = readdir(dir)){
		size_t d_name_len = strlen(dit->d_name);
		char  *ext        = &dit->d_name[ d_name_len - 4 ];

		if(ext < (char *)&dit->d_name)
			continue;
		if(strcmp(ext, ".dat") == 0){
			dit->d_name[d_name_len - 4] = '\0'; 
			db_table_load(dit->d_name);
		}
	}

	closedir(dir);
}

static void db_tables_unload(void){
	dbtable *table;
	while(table = list_pop(&db_tables))
		db_table_unload(table);
}

static void db_name_sanitize(char *name){
	char *bad_char;
	
	while(1){ bad_char = index(name, '.');  if(bad_char == NULL) break; *bad_char = '_'; }
	while(1){ bad_char = index(name, '/');  if(bad_char == NULL) break; *bad_char = '_'; }
	while(1){ bad_char = index(name, '\\'); if(bad_char == NULL) break; *bad_char = '_'; }
}

dbtable* db_tables_search(char *name){
	dbtable *table = NULL;
	
	db_name_sanitize(name);

	list_iter(&db_tables, &db_iter_table_search, (void *)name, (void *)&table);
	if(table != NULL)
		return table;
	
	return db_table_load(name);
}
/* }}}1 */
/* DB initialisation {{{1 */
/* DB settings {{{2 */
static void db_settings_save(void){
	char *dbfile_main = db_abspath("database", "set");
	FILE *db_f        = fopen(dbfile_main, "w");
	
	if(db_f == NULL){
		fprintf(stderr, "Db directory not exist or permission error\n");
		exit(EXIT_FAILURE);
	}

	fprintf(db_f, "%ld\n", db_oid_last);
	
	fclose(db_f);
	free(dbfile_main);
}

static void db_settings_load(void){
	int          res;
	struct stat  file_stat;
	char         *dbfile_main = db_abspath("database", "set");
	
	if(stat(dbfile_main, &file_stat) != 0){
		printf("db doesn't exist. creating a new one\n");
		
		/* set up default settings for database */
		db_oid_last = 0;

		/* save settings */
		db_settings_save();
	}else{
		FILE* db_f = fopen(dbfile_main, "r");
		
		if(db_f == NULL){
			fprintf(stderr, "Db directory not exist or permission error\n");
			exit(EXIT_FAILURE);
		}
		
		res = fscanf(db_f, "%ld\n", &db_oid_last);
	
		fclose(db_f);
	}
	free(dbfile_main);
}
/* }}}2 */

void db_init(void){
	list_init(&db_tables);
	
	pthread_mutex_init(&db_oid_last_mutex,  NULL);
	pthread_mutex_init(&db_mutex_query_init, NULL);
	
	db_settings_load();
	db_tables_load();
}

void db_destroy(void){
	db_settings_save();
	db_tables_unload();
}
/* }}}1 */
/* Callbacks {{{1 */
static void* db_iter_table_search(void *table, void *name, void *addr){
	if(strcmp(((dbtable *)table)->name, name) == 0){
		*(dbtable **)addr = table;
		return (void *)0;
	}
	return (void *)1; // keep searching
}

static void* db_iter_query_delete_oid(void *ptable, void *oid, void *null){
	dbtable *table = (dbtable *)ptable;
	dbitem  *item  = dbitem_alloc();
	
	item->table     = table;
	item->oid       = *(unsigned long *)oid;
	
	if(settings.verbose > 1)
		printf("removing from '%s'\n", table->name);
	
	db_table_query_delete(item);
	dbitem_free(item);
	
	return (void *)1;
}

static void* db_iter_query_get_oid(void *ptable, void *pitem, void *null){
	int      ret;
	dbtable *table = (dbtable *)ptable;
	dbitem  *item  = (dbitem  *)pitem;
	
	item->table    = table;
	
	ret = db_table_query_isset(item);
	
	if(ret == 0){
		dbitem_append(item, table->name, strlen(table->name));
	}
	
	return (void *)1;
}

static char* itoa(unsigned int num){
	char num_str[30];
	
	sprintf(num_str, "%u", num);
	
	return strdup(num_str);
}

static void* db_iter_query_search(void *item_key, void *item_value, void *pitem, void *null){
	dbitem *item = (dbitem *)pitem;
	char   *info = itoa(*(unsigned int *)item_key);
	
	dbitem_append(item, info, strlen(info));
	
	free(info);
	return (void *)1;
}

static void* db_iter_query_init(void *item_key, void *item_value, void *ptable, void *tmp_table){
	dbitem  *item  = dbitem_alloc();
	dbtable *table = (dbtable *)ptable;
	
	item->table = table;
	item->oid   = (unsigned long)*(unsigned int *)item_key;
	
	if(db_table_query_get(item) == 0){
		item->table = tmp_table;

		db_table_query_set(item);
	}
	
	dbitem_free(item);
	return (void *)1;
}

static void* db_iter_query_index(void *item_key, void *item_value, void *ptable, void *null){
	dbindex_insert(&(((dbtable *)ptable)->idx_data), item_key, NULL);
	return (void *)1;
}

static int db_cmpfunc_idx_data(dbindex *index, void *item1_key, void *item2_key){
	dbtable *table = (dbtable *)index->user_data;
	
	return data_cmp_binary(table->data_type, item1_key, item2_key);
}

static void* db_keyconv_idx_data(dbindex *index, void *item_key){
	unsigned long       item_offset;
	char               *item_ptr;
	dbtable            *table        = (dbtable *)index->user_data;
	
	if(dbindex_query(&table->idx_oid, item_key, &item_offset) != 0)
		return NULL;
	
	item_ptr = table->data.data + item_offset;
	
	return item_ptr;
}


/* }}}1 */

/* PUBLIC FUNCTIONS */

/* DBItem functions {{{1 */
dbitem* dbitem_alloc(void){
	return zalloc(sizeof(dbitem));
}

void dbitem_append(dbitem *item, void *data, unsigned long data_len){
	unsigned long  need_len;
	char          *info;
	
	need_len = data_len + 2;

	if(item->data == NULL){
		item->data_len  = need_len;
		item->data      = malloc(need_len);
		info            = item->data;
	}else{
		item->data      = realloc(item->data, item->data_len + need_len);
		info            = item->data + item->data_len;
		item->data_len += need_len;
	}
	memcpy(info, data, data_len);
	info[need_len - 1] = '\n';
	info[need_len - 2] = '\r';
}

void dbitem_free(dbitem *item){
	if(item->query)
		free(item->query);
	if(item->attribute)
		free(item->attribute);
	if(item->data)
		free(item->data);
	free(item);
}
/* }}}1 */
/* DB query functions {{{1 */
unsigned long db_query_new(void){
	unsigned long oid_new;

	if(settings.verbose > 1)
		printf("db_query_new\n");
	
	pthread_mutex_lock(&db_oid_last_mutex);
	oid_new = db_oid_last++;
	pthread_mutex_unlock(&db_oid_last_mutex);
	return oid_new;
}

unsigned int db_query_set(dbitem *item){
	
	if(settings.verbose > 1)
		printf("db_query_set\n");

	//list_push(&db_write_queue, item);	
	return db_table_query_set(item);
}

unsigned int db_query_get(dbitem *item){
	if(settings.verbose > 1)
		printf("db_query_get\n");
	
	if(item->table == NULL){
		list_iter(&db_tables, &db_iter_query_get_oid, item, NULL);
		return 0;
	}else{
		return db_table_query_get(item);	
	}
}

unsigned int db_query_delete(dbitem *item){
	if(settings.verbose > 1)
		printf("db_query_delete\n");
	
	if(item->table == NULL){
		list_iter(&db_tables, &db_iter_query_delete_oid, &item->oid, NULL);
		return 0;
	}else{
		return db_table_query_delete(item);
	}
}

unsigned int db_query_search(dbitem *item){
	unsigned int ret = 1;

	if(settings.verbose > 1)
		printf("db_query_search\n");
	
	if(item->query == NULL || strcmp(item->query, "%") == 0){
		dbindex_iter(&item->table->idx_oid, &db_iter_query_search, item, NULL);
		return 0;
	}

	if(item->table->idx_data.loaded == 1){
		/* direct value search */
		
		/* we pack item->query to binary data and try to find it */
		int            data_type  = item->table->data_type;
		unsigned long  entry_len  = data_packed_len(data_type, item->query, strlen(item->query));
		char          *entry_data = zalloc(entry_len + 1);
		
		data_pack(data_type, item->query, entry_data, entry_len);
		
		/* start searching */
		dbindex   *idx_data     = &item->table->idx_data;
		char      *item_oid;
		vm_cursor *cursor;
		
		cursor = dbindex_search(&item->table->idx_data, entry_data);
		if(cursor != NULL){
			do{
				item_oid = itoa(*(unsigned int *)cursor->real_ptr);	
				dbitem_append(item, item_oid, strlen(item_oid));
				free(item_oid);
				
			}while(dbindex_search_slide(cursor, 1) == 0);
			
			dbindex_vm_cursor_free(cursor);
			ret = 0;
		}
	
		free(entry_data);
	}
	return ret;
}

unsigned int db_query_init(dbtable *table, int type){
	pthread_mutex_lock(&db_mutex_query_init);
		dbtable *tmp_table;
		
		db_table_unlink("_temp");
		
		tmp_table = db_table_load("_temp");
		tmp_table->data_type = type;
		
		dbindex_iter(&table->idx_oid, &db_iter_query_init, table, tmp_table);
		
		db_table_rename("_temp", table->name);
		db_table_unlink("_temp");
		
	pthread_mutex_unlock(&db_mutex_query_init);
}

unsigned int db_query_deindex(dbtable *table){
	if(table->idx_data.loaded == 1){
		dbindex_unload(&table->idx_data);
	}
	
	char *file = db_abspath(table->name, "idx1"); unlink(file); free(file);
	return 0;
}

unsigned int db_query_index(dbtable *table, int type){
	int ret = 1;
	
	db_query_deindex(table);
	
	char *file = db_abspath(table->name, "idx1");
	
	if(dbindex_create(file, DBINDEX_TYPE_INDEX, 4, 0) == 0){
		db_table_load_idx_data(table);
		
		// reinsert items
		dbindex_iter(&table->idx_oid, &db_iter_query_index, table, NULL);
		
		ret = 0;
	}
	
	free(file);

	return ret;
}

unsigned int db_query_destroy(dbtable *table){
	db_table_unlink(table->name);
	return 0;
}

/* }}}1 */

/* Trash {{{1 */

//static list             db_write_queue;
//static pthread_t        db_write_thread_id;

/*void db_write_thread(int arg){
	do{
		dbitem *item;
		while(item = list_pop(&db_write_queue)){
			db_table_query_set(item);
			dbitem_free(item);
		}
		sleep(1);
	}while(daemon_quit != 1);
	db_destroy_ok++;
}*/
	/*
	if(pthread_create(&db_write_thread_id, NULL, (void *)&db_write_thread, NULL) != 0) {
		printf("db_write_thread failed\n");
		exit(EXIT_FAILURE);
	}*/
// wait for workers to destroy
	//while(db_destroy_ok < 1)
	//	sleep(1);

//list_init(&db_write_queue);
	
/* }}}1 */
