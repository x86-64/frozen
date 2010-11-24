#define FUSE_USE_VERSION 26

#include <libfrozen.h>
#include <fuse.h>

/* typedefs {{{ */
typedef struct my_data {
	char     *mountpoint;
	char     *datadir;
} my_data;

typedef enum virt_type {
	VIRTFS_DIRECTORY,
	VIRTFS_FILE,
	
	VIRTFS_INVALID = -1
} virt_type;

typedef struct virtfs_item virtfs_item;

struct virtfs_item {
	virtfs_item   *next;
	virtfs_item   *childs;
	
	virt_type      type;
	char          *name;
	char          *fullname;
	
	// fs related
	struct stat    stat;
};

#define FUSE_DATA ((my_data *) fuse_get_context()->private_data)
/* }}} */
/* global variables {{{ */
backend_t *backend;

virtfs_item  virtfs = {
	.next       = NULL,
	.childs     = NULL,
	.type       = VIRTFS_DIRECTORY,
	.name       = "",
	.fullname   = "/",
	.stat       = {
		.st_mode = S_IFDIR | 0755,
		.st_size = 0,
		.st_nlink = 1
	}
};
/* }}} */

/* stubs {{{
static int fusef_flush(const char *path, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fusef_statfs(const char *path, struct statvfs *stat){
	return 0;
}

static int fusef_readlink(const char *path, char *buf, size_t size){
	return -ENOSYS;
}

static int fusef_mknod(const char *path, mode_t mode, dev_t dev){
	return -ENOSYS;
}

static int fusef_mkdir(const char *path, mode_t mode){
	return -ENOSYS;
}

static int fusef_unlink(const char *path){
	return -ENOSYS;
}

static int fusef_rmdir(const char *path){
	return -ENOSYS;
}

static int fusef_symlink(const char *from, const char *to){
	return -ENOSYS;
}

static int fusef_rename(const char *from, const char *to){
	return -ENOSYS;
}

static int fusef_link(const char *from, const char *to){
	return -ENOSYS;
}

static int fusef_chmod(const char *path, mode_t mode){
	return -ENOSYS;
}

static int fusef_chown(const char *path, uid_t uid, gid_t gid){
	return -ENOSYS;
}

static int fusef_truncate(const char *path, off_t off){
	return -ENOSYS;
}

static int fusef_fsync(const char *path, int datasync, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fusef_setxattr(const char *path, const char *name, const char *value, size_t size, int pos){
	return -ENOSYS;
}

static int fusef_getxattr(const char *path, const char *name, char *value, size_t size){
	return -ENOSYS;
}

static int fusef_listxattr(const char *path, char *list, size_t size){
	return -ENOSYS;
}

static int fusef_removexattr(const char *path, const char *name){
	return -ENOSYS;
}

static int fusef_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fusef_access(const char *path, int mask){
	return -ENOSYS;
}

static int fusef_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fusef_ftruncate(const char *path, off_t off, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fusef_fgetattr(const char *path, struct stat *buf, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fusef_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *lock){
	return -ENOSYS;
}

static int fusef_utimens(const char *path, const struct timespec tv[2]){
	return -ENOSYS;
}

static int fusef_bmap(const char *path, size_t blocksize, uint64_t *idx){
	return -ENOSYS;
}

static int fusef_opendir(const char *path, struct fuse_file_info *fi){
	return 0;
}
static int fusef_releasedir(const char *path, struct fuse_file_info *fi){
	return 0;
}
 }}} */

static int            match_path(char *folderpath, char *filepath){
	char c1, c2;
	int  folder_len = 0;
	
	do{
		c1 = *folderpath++;
		c2 = *filepath++;
		
		if(c2 == 0)
			goto invalid;
		if(c1 == 0)
			goto check_slash;
		
		folder_len++;
	}while(c1 == c2);
invalid:
	return 0;

check_slash:
	if(folder_len == 1 || c2 == '/')
		return (strchr(filepath, '/') == NULL) ? 2 : 1;
	return 0;
}

static virtfs_item *  virtfs_create(char *path, virt_type type){
	char           *name;   
	size_t          path_len;
	virtfs_item    *folder;
	
	if( (name = strrchr(path, '/')) == NULL)
		return NULL;
	name++;
	path_len = path - name;
	
	for(folder = &virtfs; folder != NULL; folder = folder->next){
	redo:
		if(folder->type != VIRTFS_DIRECTORY)
			continue;
		
		switch(match_path(folder->fullname, path)){
			case 0:
				continue;
			case 1:
				folder = folder->childs;
				goto redo;
			case 2:
				//printf("creating %s under %s\n", path, folder->fullname);
				
				virtfs_item *new_item = calloc(sizeof(virtfs_item), 1);
				if(new_item == NULL)
					return NULL;
				
				new_item->type         = type;
				new_item->name         = strdup(name);
				new_item->fullname     = strdup(path);
				
				mode_t new_mode;
				switch(type){
					case VIRTFS_DIRECTORY: new_mode = S_IFDIR | 0755; break;
					case VIRTFS_FILE:      new_mode = S_IFREG | 0444; break;
					case VIRTFS_INVALID:   new_mode = 0; break;
				};
				new_item->stat.st_mode  = new_mode;
				new_item->stat.st_nlink = 1;
				
				// link it
				new_item->next = folder->childs;
				folder->childs = new_item;
				break;
		};
	}
	return NULL;
}

static virtfs_item *  virtfs_find(char *path){
	virtfs_item    *folder;
	
	//printf("find: %s\n", path);
	for(folder = &virtfs; folder != NULL; folder = folder->next){
	redo:
		//printf("find: do: %s\n", folder->fullname);
		
		if(strcmp(folder->fullname, path) == 0)
			return folder;
		
		if(
			folder->type == VIRTFS_DIRECTORY &&
			match_path(folder->fullname, path) != 0
		){
			//printf("find: move into %s\n", folder->fullname);
			folder = folder->childs;
			goto redo;
		}
	}
	return NULL;
}

/* folders {{{ */
static int fusef_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi){
	virtfs_item *folder, *child;
	
	if( (folder = virtfs_find((char *)path)) == NULL)
		return -ENOENT;
	
	filler(buf, ".",  NULL, 0);
	filler(buf, "..", NULL, 0);
	
	child = folder->childs;
	while(child != NULL){
		filler(buf, child->name, NULL, 0);
		
		child = child->next;
	}
	return 0;
}

static int fusef_getattr(const char *path, struct stat *buf){
	virtfs_item *item;
	
	if( (item = virtfs_find((char *)path)) == NULL)
		return -ENOENT;
	
	memcpy(buf, &(item->stat), sizeof(struct stat));
	return 0;
}
/* }}} */
/* files {{{ 
static int fusef_open(const char *path, struct fuse_file_info *fi){
	return 0;
}

static int fusef_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi){
	request_t r_read[] = {
		{ "action", DATA_INT32(ACTION_CRWD_READ)     },
		{ "key",    DATA_OFFT(data_ptrs[i])          },
		{ "buffer", DATA_PTR_INT32(&data_read)       },
		hash_end
	};
	ret = backend_query(backend, r_read);
		fail_unless(ret == 4,                   "chain 'real_store_nums': read array failed");
		fail_unless(data_read == data_array[i], "chain 'real_store_nums': read array data failed");
	return -ENOSYS;
}

static int fusef_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi){
	request_t r_write[] = {
		{ "action",  DATA_INT32 (ACTION_CRWD_WRITE)  },
		{ "key_out", DATA_PTR_OFFT  (&data_ptrs[i])  },
		{ "buffer",  DATA_PTR_INT32 (&data_array[i]) },
		hash_end
	};
	ret = backend_query(backend, r_write);
		fail_unless(ret == 4, "chain 'real_store_nums': write array failed");
	return -ENOSYS;
}


static int fusef_release(const char *path, struct fuse_file_info *fi){
	return 0;
}
}}} */

struct fuse_operations fuse_operations = { // {{{
	/*
	.readlink = fusef_readlink,
	.mknod = fusef_mknod,
	.mkdir = fusef_mkdir,
	.unlink = fusef_unlink,
	.rmdir = fusef_rmdir,
	.symlink = fusef_symlink,
	.rename = fusef_rename,
	.link = fusef_link,
	.chmod = fusef_chmod,
	.chown = fusef_chown,
	.truncate = fusef_truncate,
	.open = fusef_open,
	.read = fusef_read,
	.write = fusef_write,
	.statfs = fusef_statfs,
	.flush = fusef_flush,
	.release = fusef_release,
	.fsync = fusef_fsync,
	.setxattr = fusef_setxattr,
	.getxattr = fusef_getxattr,
	.listxattr = fusef_listxattr,
	.removexattr = fusef_removexattr,
	.opendir = fusef_opendir,
	.releasedir = fusef_releasedir,
	.fsyncdir = fusef_fsyncdir,
	.access = fusef_access,
	.create = fusef_create,
	.ftruncate = fusef_ftruncate,
	.fgetattr = fusef_fgetattr,
	.lock = fusef_lock,
	.utimens = fusef_utimens,
	.bmap = fusef_bmap
	*/
	.readdir = fusef_readdir,
	.getattr = fusef_getattr,
}; // }}}
int main(int argc, char *argv[]){ // {{{
	my_data *data;
	int      ret;
	
	if( (data = calloc(sizeof(my_data), 1)) == NULL)
		return ENOMEM;
	
	data->mountpoint = realpath(argv[3], NULL);
	data->datadir    = realpath(argv[4], NULL);
	
	virtfs_create("/hello1",       VIRTFS_DIRECTORY);
	virtfs_create("/hello1/test1", VIRTFS_FILE);
	virtfs_create("/hello1/test2", VIRTFS_FILE);
	virtfs_create("/hello1/test3", VIRTFS_FILE);
	virtfs_create("/hello2",       VIRTFS_DIRECTORY);
	virtfs_create("/hello2/woot1", VIRTFS_FILE);
	virtfs_create("/hello2/woot2", VIRTFS_FILE);
	virtfs_create("/hello2/woot3", VIRTFS_FILE);
	virtfs_create("/hello3",       VIRTFS_FILE);
	
	/*
	frozen_init();
	hash_set(global_settings, "homedir", TYPE_STRING, data->datadir, strlen(data->datadir) + 1);
	
	hash_t config[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")        },
				{ "filename",  DATA_STRING("test.dat")    },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend = backend_new(config);
	*/
	ret = fuse_main(argc, argv, &fuse_operations, data);
	/*
	backend_destroy(backend);
	frozen_destroy();
	*/
	return ret;
} // }}}

