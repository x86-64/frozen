#include <memcachedb.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <dirent.h>
#include <linux/mman.h>

typedef struct mapping_ mapping;
struct mapping_ {
	int               fd;
	char             *data;
	long              data_len;
	long              data_mlen;
	pthread_rwlock_t  lock;
};

typedef struct dbtable_ dbtable;
struct dbtable_ {
	char           *name;
	
	/* table settings */
	int             pack_type;
	
	/* table data */
	mapping         data;
	mapping         index;
};

#include <list.c>

/* db settings */
char                   *dbfile_main;

static unsigned int     db_destroy_ok = 0;
static unsigned int     db_oid_last;
static pthread_mutex_t  db_oid_last_mutex;
static list             db_write_queue;
static list             db_tables;

static pthread_t        db_write_thread_id;

/* PUBLIC FUNCTIONS */

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


unsigned int db_query_new(void){
	unsigned int oid_new;

	pthread_mutex_lock(&db_oid_last_mutex);
	oid_new = db_oid_last++;
	pthread_mutex_unlock(&db_oid_last_mutex);
	return oid_new;
}

unsigned int db_query_set(dbitem *item){
	list_push(&db_write_queue, item);	
	return 0;
}

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

static int db_mapfile(char *path, mapping *map){
	int          fd;
	struct stat  file_stat;

	if( (fd = open(path, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR)) < 0)
		return 0;
	
	fstat(fd, &file_stat);
	
	map->fd        =  fd;
	map->data_len  =  file_stat.st_size;
	map->data_mlen = (file_stat.st_size + 0x1000) & ~0xFFF;
	pthread_rwlock_init(&map->lock, NULL);

	char *ret = mmap(0, map->data_mlen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(ret == MAP_FAILED){
		perror(path);
		return 0;
	}
	map->data = ret;
	
	return 1;
}

static void db_map_lock(mapping *map){
	pthread_rwlock_rdlock(&map->lock);
}

static void db_map_unlock(mapping *map){
	pthread_rwlock_unlock(&map->lock);
}

static long db_expandfile(mapping *map, long add_size){
	pthread_rwlock_wrlock(&map->lock);

	long old_len  = map->data_len;
	long old_mlen = map->data_mlen;

	map->data_len   = old_len + add_size;
	map->data_mlen  = (map->data_len + 0x1000) & ~0xFFF;
	
	int res;
	lseek(map->fd, map->data_len - 1, SEEK_SET);
	res = write(map->fd, "", 1);

	if(old_mlen != map->data_mlen){
		char *ret = mremap(map->data, old_mlen, map->data_mlen, MREMAP_MAYMOVE);
		if(ret == MAP_FAILED){
			perror("expandfile: mremap");
			exit(EXIT_FAILURE);
		}
		map->data = ret;
	}

	pthread_rwlock_unlock(&map->lock);

	return old_len; // offset to expanded area
}

static void db_unmapfile(mapping *map){
	pthread_rwlock_wrlock(&map->lock);

	msync  (map->data, map->data_len, MS_SYNC);
	munmap (map->data, map->data_len);
	close  (map->fd);
	
	map->data     = NULL;
	map->data_len = 0;

	pthread_rwlock_unlock(&map->lock);
}
/* }}}1 */

static void *db_iter_table_search(void *table, void *name, void *addr){
	if(strcmp(((dbtable *)table)->name, name) == 0){
		*(dbtable **)addr = table;
		return (void *)0;
	}
	return (void *)1; // keep searching
}

/* Table settings {{{2 */
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
	dbtable *table = NULL;
	
	// is already loaded?
	list_iter(&db_tables, &db_iter_table_search, name, &table);
	if(table != NULL)
		return;
	
	size_t max_len = strlen(name) + 4 + 1;

	char *apath_data = db_abspath(name, "dat");
	char *apath_idx  = db_abspath(name, "idx");
	
	table = zalloc(sizeof(dbtable));
	table->name = strdup(name);
	
	db_table_settings_load(table);

	if(
		db_mapfile(apath_data, &table->data)  == 0 ||
		db_mapfile(apath_idx,  &table->index) == 0
	){
		printf("failed to load '%s'\n", name);
		exit(EXIT_FAILURE);
	}
	free(apath_data);
	free(apath_idx);

	list_push(&db_tables, table);
	if(settings.verbose > 1)
		printf("table '%s' loaded\n", name);
	
	return table;
}

static void db_table_unload(dbtable *table){
	db_table_settings_save(table);

	db_unmapfile(&table->data);
	db_unmapfile(&table->index);
	
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

/* db_table_search return db_table struct anyway */
static dbtable* db_table_search(char *name){
	dbtable *table = NULL;
	
	list_iter(&db_tables, &db_iter_table_search, (void *)name, (void *)&table);
	if(table != NULL)
		return table;
	
	return db_table_load(name);
}

static void db_table_query_set(dbitem *item){
	dbtable *table = db_table_search(item->attribute);
	
	long data_off = db_expandfile(&table->data, item->data_len);
	db_map_lock(&table->data);
		// format as pack_type said
		memcpy(table->data.data + data_off, item->data, item->data_len);
	db_map_unlock(&table->data);
	// add to index
}

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

/* DB settings {{{1 */
static void db_settings_save(void){
	FILE* db_f = fopen(dbfile_main, "w");

	fprintf(db_f, "%u\n", db_oid_last);
	
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

		res = fscanf(db_f, "%u\n", &db_oid_last);
	
		fclose(db_f);
	}
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
		printf("db_oid = %u\n", db_oid_last);
	
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

