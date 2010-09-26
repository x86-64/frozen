#include <memcachedb.h>
#include <dbmap.h>

// TODO write dbmap_shrink functions

int dbmap_map(char *path, dbmap *map){
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

void dbmap_lock(dbmap *map){
	pthread_rwlock_rdlock(&map->lock);
}

void dbmap_unlock(dbmap *map){
	pthread_rwlock_unlock(&map->lock);
}

unsigned long dbmap_expand(dbmap *map, unsigned long add_size){
	pthread_rwlock_wrlock(&map->lock);

	unsigned long old_len  = map->data_len;
	unsigned long old_mlen = map->data_mlen;
	unsigned long new_len  = old_len + add_size;
	unsigned long new_mlen = (new_len + 0x1000) & ~0xFFF;
	
	int res;
	res = lseek(map->fd, new_len - 1, SEEK_SET);
	res = write(map->fd, "", 1);
	
	if(res == -1){
		old_len = (unsigned long) -1;
	}else{

		map->data_len  = new_len;
		map->data_mlen = new_mlen;
	
		if(old_mlen != new_mlen){
			char *ret;
			
			// not work :(
			//ret = mremap(map->data, old_mlen, map->data_mlen, MREMAP_MAYMOVE);
			
			msync  (map->data, old_mlen, MS_SYNC);
			munmap (map->data, old_mlen);
			ret = mmap(0, map->data_mlen, PROT_READ | PROT_WRITE, MAP_SHARED, map->fd, 0);
			if(ret == MAP_FAILED){
				perror("expandfile: mremap");
				exit(EXIT_FAILURE);
			}
			map->data = (char *)ret;
		}
	}
	
	pthread_rwlock_unlock(&map->lock);

	return old_len; // offset to expanded area
}

void dbmap_sync(dbmap *map){
	msync(map->data, map->data_mlen , MS_SYNC);
}

void dbmap_unmap(dbmap *map){
	pthread_rwlock_wrlock(&map->lock);

	msync  (map->data, map->data_len, MS_SYNC);
	munmap (map->data, map->data_len);
	close  (map->fd);
	
	map->data     = NULL;
	map->data_len = 0;

	pthread_rwlock_unlock(&map->lock);
}

