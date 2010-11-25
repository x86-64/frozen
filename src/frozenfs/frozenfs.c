#define FUSE_USE_VERSION 26
#define _GNU_SOURCE

#include <libfrozen.h>
#include <fuse.h>
#include <string.h>
#include <sys/time.h>

/* typedefs {{{ */
struct my_opts {
	char     *datadir;
};

typedef struct virtfs_item   virtfs_item;
typedef enum virt_type       virt_type;

enum virt_type {
	VIRTFS_DIRECTORY,
	VIRTFS_FILE,
	
	VIRTFS_INVALID = -1
};

struct virtfs_item {
	virtfs_item   *next;
	virtfs_item   *childs;
	
	virt_type      type;
	char          *name;
	char          *fullname;
	
	// fs related
	struct stat    stat;
};

enum frozenfs_keys {
	KEY_HELP,
};

/* }}} */
/* global variables {{{ */
static struct my_opts opts;

backend_t     *backend;
unsigned int   cnt_unnamed = 0;

virtfs_item  virtfs = {
	.next       = NULL,
	.childs     = NULL,
	.type       = VIRTFS_DIRECTORY,
	.name       = "",
	.fullname   = "/",
	.stat       = {
		.st_mode  = S_IFDIR | 0755,
		.st_size  = 0,
		.st_nlink = 1
	}
};

static struct fuse_opt frozenfs_opts[] = {
	FUSE_OPT_KEY("-h",             KEY_HELP),
	FUSE_OPT_KEY("--help",         KEY_HELP),
	FUSE_OPT_END
};

/* }}} */

/* virtfs {{{ */
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

static virtfs_item *  virtfs_find(char *path){
	virtfs_item    *folder;
	
	if(strcmp(path, "") == 0 || strcmp(path, "/") == 0)
		return &virtfs;
	
	//printf("find: %s\n", path);
	for(folder = &virtfs; folder != NULL;){
		//printf("find: do: %s\n", folder->fullname);
		
		if(strcmp(folder->fullname, path) == 0)
			return folder;
		
		if(
			folder->type == VIRTFS_DIRECTORY &&
			match_path(folder->fullname, path) != 0
		){
			//printf("find: move into %s\n", folder->fullname);
			folder = folder->childs;
			continue;
		}
		
		folder = folder->next;
	}
	return NULL;
}

static virtfs_item *  virtfs_create(char *path, virt_type type){
	char           *name, *fullname;
	size_t          path_len;
	virtfs_item    *folder, *new_item;
	struct timeval  tv;
	
	if( (name = strrchr(path, '/')) == NULL)
		return NULL;
	name++;
	path_len = name - path - 1;
	fullname = strndupa(path, path_len);
	
	if( (folder = virtfs_find(fullname)) == NULL)
		return NULL;
	
	if( (new_item = calloc(sizeof(virtfs_item), 1)) == NULL)
		return NULL;
	
	gettimeofday(&tv, NULL);
	
	new_item->type         = type;
	new_item->name         = strdup(name);
	new_item->fullname     = strdup(path);
	
	mode_t new_mode;
	switch(type){
		case VIRTFS_DIRECTORY: new_mode = S_IFDIR | 0755; break;
		case VIRTFS_FILE:      new_mode = S_IFREG | 0666; break;
		case VIRTFS_INVALID:   new_mode = 0; break;
	};
	new_item->stat.st_mode  = new_mode;
	new_item->stat.st_nlink = 1;
	new_item->stat.st_uid   = getuid();
	new_item->stat.st_gid   = getgid();
	new_item->stat.st_atime = tv.tv_sec;
	new_item->stat.st_mtime = tv.tv_sec;
	new_item->stat.st_ctime = tv.tv_sec;
	
	// link it
	new_item->next = folder->childs;
	folder->childs = new_item;
	return new_item;
}
/* }}} */

/* frozen wrappers {{{ */
static backend_t *  wfrozen_backend_new(hash_t *config){
	backend_t *backend;
	char      *backend_name, tmp[1024];
	
	if( (backend = backend_new(config)) == NULL)
		return NULL;
	
	if( (backend_name = backend_get_name(backend)) == NULL){
		snprintf(tmp, 1024, "/backends/unnamed_%d", cnt_unnamed++);
	}else{
		snprintf(tmp, 1024, "/backends/%s", backend_name);
	}
	virtfs_create(tmp, VIRTFS_FILE);
	
	return backend;
};

static void         wfrozen_backend_destory(backend_t *backend){
	backend_destroy(backend);
};

static void wfrozen_init(void){
	virtfs_create("/backends",     VIRTFS_DIRECTORY);
	virtfs_create("/config",       VIRTFS_FILE);
	
	frozen_init();
	
	hash_set(global_settings, "homedir", TYPE_STRING, opts.datadir, strlen(opts.datadir) + 1);
	
	hash_t config[] = {
		{ NULL,    DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                 },
				{ "filename",  DATA_STRING("data_ffs_test.dat")    },
				hash_end
			)},
			hash_end
		)},
		{ "name",  DATA_STRING("be_file")                         },
		hash_end
	};
	
	backend = wfrozen_backend_new(config);
}

static void wfrozen_destroy(void){
	wfrozen_backend_destory(backend);

	frozen_destroy();
}
/* }}} */

/* FUSE functions {{{ */
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
/* }}} */

/* options and usage {{{ */
static void usage(void){
	printf("usage: frozenfs datadir mountpoint [options]\n");
}

static int frozenfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs){
	switch(key){
		case FUSE_OPT_KEY_OPT:
			//printf("fuse: key opt: %s\n", arg);
				//tmp = g_strdup_printf("-o%s", arg);
				//ssh_add_arg(tmp);
				//g_free(tmp);
				//return 0;
			return 1;
		
		case FUSE_OPT_KEY_NONOPT:
			if(!opts.datadir){
				opts.datadir = strdup(arg);
				return 0;
			}
			return 1;
		
		case KEY_HELP:
			usage();
			fuse_opt_add_arg(outargs, "-ho");
			fuse_main(outargs->argc, outargs->argv, NULL, NULL);
			exit(1);
			
		default:
			return -1;
	};
}

int main(int argc, char *argv[]){
	int              ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	
	if(fuse_opt_parse(&args, &opts, frozenfs_opts, frozenfs_opt_proc) == -1)
		exit(1);
	
	if(!opts.datadir){
		fprintf(stderr, "missing datadir");
		exit(1);
	}
	
	wfrozen_init();
	ret = fuse_main(args.argc, args.argv, &fuse_operations, NULL);
	wfrozen_destroy();
	return ret;
}
/* }}} */

