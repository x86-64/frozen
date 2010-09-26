#include <memcachedb.h>
#include <list.h>
#include <dbmap.h>
#include <dbindex.h>
#include <db.h>

/* db settings */
char                   *dbfile_main;

static unsigned int     db_destroy_ok = 0;
static unsigned long    db_oid_last;
static pthread_mutex_t  db_oid_last_mutex;
static list             db_write_queue;
static list             db_tables;

static pthread_t        db_write_thread_id;

static void* db_iter_table_search(void *table, void *name, void *addr);
static void* db_iter_query_delete_oid(void *table, void *oid, void *null);

// TODO: test 1-4 pack_types

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

/* DBTable initialisation {{{1 */
/* DBTable settings {{{2 */
static void db_table_settings_save(dbtable *table){
	char *apath_set  = db_abspath(table->name, "set");

	FILE *fp_set = fopen(apath_set, "w");
	
	fprintf(fp_set, "%d\n", table->pack_type);
	
	fclose(fp_set);
}

static void db_table_settings_load(dbtable *table){
	int          res;
	struct stat  file_stat;

	char *apath_set  = db_abspath(table->name, "set");
	if(stat(apath_set, &file_stat) != 0){
		table->pack_type = 5;
		
		db_table_settings_save(table);
	}else{
		FILE *fp_set = fopen(apath_set, "r");

		res = fscanf(fp_set, "%d\n", &table->pack_type);

		fclose(fp_set);
	}
}
/* }}}2 */

static dbtable* db_table_load(char *name){
	dbtable     *table = NULL;
	struct stat  index_stat;

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
		dbindex_create(apath_idx_oid, DBINDEX_TYPE_PRIMARY, 4, 8);
	}
	dbindex_load(apath_idx_oid, &table->idx_oid);
	free(apath_idx_oid);


	list_push(&db_tables, table);
	if(settings.verbose > 1)
		printf("table '%s' loaded\n", name);
	return table;
}

static void db_table_unload(dbtable *table){
	db_table_settings_save(table);

	dbindex_unload(&table->idx_oid);
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

/* }}}1 */

dbtable* db_tables_search(char *name){
	if(settings.verbose > 1)
		printf("db_tables_search\n");

	dbtable *table = NULL;
	
	list_iter(&db_tables, &db_iter_table_search, (void *)name, (void *)&table);
	if(table != NULL)
		return table;
	
	return db_table_load(name);
}

static void db_table_query_set(dbitem *item){
	dbtable *table = item->table;
	
	char          *entry_ptr;
	unsigned long  entry_off;
	unsigned int   entry_len;
	switch(table->pack_type){
		case 1:  entry_len = 1; break;                      // byte
		case 2:  entry_len = 4; break;                      // u32
		case 3:  entry_len = 8; break;                      // u64
		case 4:  entry_len = strlen(item->data) + 1; break; // data\0 - use strlen coz client can cheat and supply
			                                            // data with \0 in it, which can lead to wasting disk space
		default: entry_len = item->data_len + 4;     break; // u32 len; data
	};
	
	//if search free space failed
	entry_off = dbmap_expand(&table->data, entry_len);
	
	dbmap_lock(&table->data);
		unsigned long long data_ll = 0;
			
		entry_ptr = table->data.data + entry_off;

		switch(table->pack_type){
			case 1:
			case 2:
			case 3:
				data_ll = strtoull(item->data, NULL, 10);
				memcpy(entry_ptr, &data_ll, entry_len);
				break;
			case 4:
				memcpy(entry_ptr, item->data, entry_len);
				break;
			default:
				*(unsigned int *)entry_ptr = item->data_len;
				memcpy(entry_ptr + 4, item->data, item->data_len);
				break;
		};
	dbmap_unlock(&table->data);
	
	if(settings.verbose > 1)
		printf("oid: %ld off: %ld\n", item->oid, entry_off);
	dbindex_insert(&table->idx_oid, &item->oid, &entry_off);
	
}

static unsigned int db_table_query_isset(dbitem *item){
	unsigned long entry_off;
	
	return dbindex_query(&item->table->idx_oid, &item->oid, &entry_off);
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
		switch(table->pack_type){
			case 1:
			case 2:
			case 3:
				item->data = malloc(30);
				break;
			case 4:
				entry_len  = strlen(entry_ptr) + 1;
				item->data = malloc(entry_len);
				break;
			default:
				entry_len  = *(unsigned int *)entry_ptr;
				item->data = malloc(entry_len);
				entry_ptr += 4;
				break;
		};
		
		switch(table->pack_type){
			case 1: sprintf(item->data,   "%d", *entry_ptr);                       break;
			case 2: sprintf(item->data,   "%d", *(unsigned int *)entry_ptr);       break;
			case 3: sprintf(item->data, "%lld", *(unsigned long long *)entry_ptr); break;
			default:
				memcpy(item->data, entry_ptr, entry_len);
				break;
		}
		
		switch(table->pack_type){
			case 1:
			case 2:
			case 3:
				item->data_len = strlen(item->data);
				break;
			default:
				item->data_len = entry_len;
				break;
		};

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
		
		ret = dbindex_delete(&item->table->idx_oid, &item->oid);
		
		return ret;
	}
	return 1;
}

/* Tables initialisation {{{1 */
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
/* }}}1 */
/* DB workers {{{1 */
void db_write_thread(int arg){
	do{
		dbitem *item;
		while(item = list_pop(&db_write_queue)){
			db_table_query_set(item);
			dbitem_free(item);
		}
		sleep(1);
	}while(daemon_quit != 1);
	db_destroy_ok++;
}
/* }}}1 */
/* DB initialisation {{{1 */
/* DB settings {{{2 */
static void db_settings_save(void){
	FILE* db_f = fopen(dbfile_main, "w");

	fprintf(db_f, "%ld\n", db_oid_last);
	
	fclose(db_f);
}

static void db_settings_load(void){
	int res;
	struct stat file_stat;

	if(stat(dbfile_main, &file_stat) != 0){
		printf("db doesn't exist. createing a new one\n");
		
		/* set up default settings for database */
		db_oid_last = 0;

		/* save settings */
		db_settings_save();
	}else{
		FILE* db_f = fopen(dbfile_main, "r");

		res = fscanf(db_f, "%ld\n", &db_oid_last);
	
		fclose(db_f);
	}
}
/* }}}2 */

void db_init(void){
	
	list_init(&db_write_queue);
	list_init(&db_tables);

	pthread_mutex_init(&db_oid_last_mutex, NULL);

        if(pthread_create(&db_write_thread_id, NULL, (void *)&db_write_thread, NULL) != 0) {
		printf("db_write_thread failed\n");
		exit(EXIT_FAILURE);
	}

	/* load database settings */
	if(settings.verbose > 0)
		printf("loading db %s\n", settings.dbpath);
	
	dbfile_main = db_abspath("database", "set");

	db_settings_load();
	db_tables_load();

	if(settings.verbose > 1)
		printf("db_oid = %ld\n", db_oid_last);
	
}

void db_destroy(void){
	// wait for workers to destroy
	while(db_destroy_ok < 1)
		sleep(1);

	// save settings and stuff
	db_settings_save();
	
	// free all lists and variables
	db_tables_unload();
	free(dbfile_main);
}
/* }}}1 */
/* Iterators {{{1 */
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
		unsigned int  need_len = strlen(table->name) + 2;
		char         *info;
		
		if(item->data == NULL){
			item->data_len  = need_len;
			item->data      = malloc(need_len);
			info            = item->data;
		}else{
			item->data      = realloc(item->data, item->data_len + need_len);
			info            = item->data + item->data_len;
			item->data_len += need_len;
		}
		strcpy(info, table->name);
		info[need_len - 1] = '\n';
		info[need_len - 2] = '\r';
	}
	
	return (void *)1;
}
/* }}}1 */

/* PUBLIC FUNCTIONS */

/* DBItem functions {{{1 */
dbitem* dbitem_alloc(void){
	return zalloc(sizeof(dbitem));
}

void dbitem_free(dbitem *item){
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
	db_table_query_set(item);
	return 0;
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

/* }}}1 */
