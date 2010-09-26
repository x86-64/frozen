typedef struct dbmap_ dbmap;
struct dbmap_ {
	int               fd;
	char             *data;
	unsigned long     data_len;
	unsigned long     data_mlen;
	pthread_rwlock_t  lock;
};

int            dbmap_map(char *path, dbmap *map);
void           dbmap_lock(dbmap *map);
void           dbmap_unlock(dbmap *map);
void           dbmap_sync(dbmap *map);
unsigned long  dbmap_expand(dbmap *map, unsigned long add_size);
void           dbmap_unmap(dbmap *map);

