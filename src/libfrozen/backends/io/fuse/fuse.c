#define FUSE_USE_VERSION 26
#define _GNU_SOURCE

#include <libfrozen.h>
#include <fuse.h>
#include <string.h>
#include <time.h>

#define EMODULE 28

typedef enum vfs_item_type {
	VFS_FOLDER,
	VFS_FILE
} vfs_item_type;

typedef struct vfs_item            vfs_item;
struct vfs_item {
	vfs_item              *next;
	vfs_item              *childs;
	
	vfs_item_type          type;
	char                  *name;
	char                  *path;
	
	backend_t             *backend;
	
	// fs related
	struct stat            stat;
};

vfs_item  vfs = {
	.next       = NULL,
	.childs     = NULL,
	.type       = VFS_FOLDER,
	.name       = "",
	.path   = "/",
	.stat       = {
		.st_mode  = S_IFDIR | 0755,
		.st_size  = 0,
		.st_nlink = 1
	}
};

/* vfs {{{ */
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

static vfs_item *  vfs_find(char *path, size_t path_len){
	vfs_item    *folder;
	
	if(strcmp(path, "") == 0 || strcmp(path, "/") == 0)
		return &vfs;
	
	//printf("find: %s\n", path);
	for(folder = &vfs; folder != NULL;){
		//printf("find: do: %s\n", folder->path);
		
		if(strncmp(folder->path, path, path_len) == 0)
			return folder;
		
		if(
			folder->type == VFS_FOLDER &&
			match_path(folder->path, path) != 0
		){
			//printf("find: move into %s\n", folder->path);
			folder = folder->childs;
			continue;
		}
		
		folder = folder->next;
	}
	return NULL;
}

static vfs_item *  vfs_item_create(char *path, vfs_item_type vfs_type, backend_t *backend){
	size_t                 path_len;
	char                  *name;
	vfs_item              *folder, *new_item;
	
	if( (name = strrchr(path, '/')) == NULL)
		return NULL;
	name++;
	path_len = name - path - 1;
	
	if( (folder = vfs_find(path, path_len)) == NULL)
		return NULL;
	
	if( (new_item = calloc(sizeof(vfs_item), 1)) == NULL)
		return NULL;
	
	new_item->type         = vfs_type;
	new_item->path         = strdup(path);
	new_item->name         = strrchr(path, '/') + 1;
	
	mode_t new_mode;
	switch(vfs_type){
		case VFS_FOLDER:    new_mode = S_IFDIR | 0755; break;
		case VFS_FILE:      new_mode = S_IFREG | 0666; break;
	};
	new_item->stat.st_mode  = new_mode;
	new_item->stat.st_nlink = 1;
	new_item->stat.st_uid   = getuid();
	new_item->stat.st_gid   = getgid();
	new_item->stat.st_atime = 
	new_item->stat.st_mtime =
	new_item->stat.st_ctime = time(NULL);
	
	new_item->backend = backend;
	
	// link it
	new_item->next = folder->childs;
	folder->childs = new_item;
	return new_item;
}
/* }}} */
/* folders {{{ */
static int fuseh_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi){
	vfs_item              *folder, *child;
	
	if( (folder = vfs_find((char *)path, ~0)) == NULL)
		return -ENOENT;
	
	filler(buf, ".",  NULL, 0);
	filler(buf, "..", NULL, 0);
	
	// TODO check
	//child = folder->childs;
	//while(child != NULL){
	//	filler(buf, child->name, NULL, 0);
	//	
	//	child = child->next;
	//}
	for(child = folder->childs; child != NULL; child = child->next)
		filler(buf, child->name, NULL, 0);
	return 0;
}

static int fuseh_getattr(const char *path, struct stat *buf){
	vfs_item              *item;
	off_t                  size              = 0;
	
	if( (item = vfs_find((char *)path, ~0)) == NULL)
		return -ENOENT;
	
	if(item->type == VFS_FILE){
		// updating size
		request_t r_count[] = {
			{ HK(action),  DATA_UINT32T(ACTION_CRWD_COUNT)       },
			{ HK(buffer),  DATA_PTR_OFFT(&size)                  },
			hash_end
		};
		backend_query(item->backend, r_count);
	}
	item->stat.st_size = size; //(param) ?
	//	((offset > size) ? 0 : size - offset) :
	//	size;
	
	memcpy(buf, &(item->stat), sizeof(struct stat));
	return 0;
}
/* }}} */
/* files {{{ */ 
static int fuseh_open(const char *path, struct fuse_file_info *fi){
	vfs_item              *item;
	
	if( (item = vfs_find((char *)path, ~0)) == NULL)
		return -ENOENT;
	
	// TODO check access
	// TODO lock item
	
	fi->fh = (unsigned long)item;
	return 0;
}

static int fuseh_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi){
	ssize_t                ret;
	vfs_item              *item              = (vfs_item *)(uintptr_t)fi->fh;
	
	request_t r_read[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_READ)                  },
		{ HK(offset), DATA_PTR_OFFT(&off)                             },
		{ HK(buffer), DATA_RAW(buf, size)                             },
		{ HK(path),   DATA_PTR_STRING_AUTO((char *)path)              },
		hash_end
	};
	ret = backend_query(item->backend, r_read);
	
	printf("read: %x bytes from %x. ret: %x, data: %s\n", (int)size, (int)off, (int)ret, buf);
	
	return (ret < 0) ? 0 : (int)ret; //(int)size;
}

static int fuseh_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi){
	ssize_t                ret;
	vfs_item              *item              = (vfs_item *)(uintptr_t)fi->fh;
	
	request_t r_write[] = {
		{ HK(action),  DATA_UINT32T(ACTION_CRWD_WRITE)                 },
		{ HK(offset),  DATA_PTR_OFFT(&off)                             },
		{ HK(buffer),  DATA_RAW((char *)buf, size)                     },
		{ HK(path),    DATA_PTR_STRING_AUTO((char *)path)              },
		hash_end
	};
	ret = backend_query(item->backend, r_write);
	
	printf("write: %x bytes from %x. ret: %x\n", (int)size, (int)off, (int)ret);
	
	return (ret < 0) ?
		0 :
		((ret > (ssize_t)size) ? size : (int)ret);
}

static int fuseh_release(const char *path, struct fuse_file_info *fi){
	// TODO unlock item
	return 0;
}
/* }}} */
/* stubs {{{
static int fuseh_flush(const char *path, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuseh_statfs(const char *path, struct statvfs *stat){
	return 0;
}

static int fuseh_readlink(const char *path, char *buf, size_t size){
	return -ENOSYS;
}

static int fuseh_mknod(const char *path, mode_t mode, dev_t dev){
	return -ENOSYS;
}

static int fuseh_mkdir(const char *path, mode_t mode){
	return -ENOSYS;
}

static int fuseh_unlink(const char *path){
	return -ENOSYS;
}

static int fuseh_rmdir(const char *path){
	return -ENOSYS;
}

static int fuseh_symlink(const char *from, const char *to){
	return -ENOSYS;
}

static int fuseh_rename(const char *from, const char *to){
	return -ENOSYS;
}

static int fuseh_link(const char *from, const char *to){
	return -ENOSYS;
}

static int fuseh_chmod(const char *path, mode_t mode){
	return -ENOSYS;
}

static int fuseh_chown(const char *path, uid_t uid, gid_t gid){
	return -ENOSYS;
}

static int fuseh_truncate(const char *path, off_t off){
	return -ENOSYS;
}

static int fuseh_fsync(const char *path, int datasync, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuseh_setxattr(const char *path, const char *name, const char *value, size_t size, int pos){
	return -ENOSYS;
}

static int fuseh_getxattr(const char *path, const char *name, char *value, size_t size){
	return -ENOSYS;
}

static int fuseh_listxattr(const char *path, char *list, size_t size){
	return -ENOSYS;
}

static int fuseh_removexattr(const char *path, const char *name){
	return -ENOSYS;
}

static int fuseh_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuseh_access(const char *path, int mask){
	return -ENOSYS;
}

static int fuseh_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuseh_ftruncate(const char *path, off_t off, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuseh_fgetattr(const char *path, struct stat *buf, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuseh_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *lock){
	return -ENOSYS;
}

static int fuseh_utimens(const char *path, const struct timespec tv[2]){
	return -ENOSYS;
}

static int fuseh_bmap(const char *path, size_t blocksize, uint64_t *idx){
	return -ENOSYS;
}

static int fuseh_opendir(const char *path, struct fuse_file_info *fi){
	return 0;
}
static int fuseh_releasedir(const char *path, struct fuse_file_info *fi){
	return 0;
}
 }}} */
struct fuse_operations fuse_operations = { // {{{
	/*
	.readlink = fuseh_readlink,
	.mknod = fuseh_mknod,
	.mkdir = fuseh_mkdir,
	.unlink = fuseh_unlink,
	.rmdir = fuseh_rmdir,
	.symlink = fuseh_symlink,
	.rename = fuseh_rename,
	.link = fuseh_link,
	.chmod = fuseh_chmod,
	.chown = fuseh_chown,
	.truncate = fuseh_truncate,
	.statfs = fuseh_statfs,
	.flush = fuseh_flush,
	.release = fuseh_release,
	.fsync = fuseh_fsync,
	.setxattr = fuseh_setxattr,
	.getxattr = fuseh_getxattr,
	.listxattr = fuseh_listxattr,
	.removexattr = fuseh_removexattr,
	.opendir = fuseh_opendir,
	.releasedir = fuseh_releasedir,
	.fsyncdir = fuseh_fsyncdir,
	.access = fuseh_access,
	.create = fuseh_create,
	.ftruncate = fuseh_ftruncate,
	.fgetattr = fuseh_fgetattr,
	.lock = fuseh_lock,
	.utimens = fuseh_utimens,
	.bmap = fuseh_bmap
	*/
	.readdir = fuseh_readdir,
	.getattr = fuseh_getattr,
	.open    = fuseh_open,
	.read    = fuseh_read,
	.write   = fuseh_write,
	.release = fuseh_release,
}; // }}}

static int fuseb_init(backend_t *backend){ // {{{
	//if((backend->userdata = calloc(1, sizeof(fuse_userdata))) == NULL)
	//	return error("calloc failed");
	
	return 0;
} // }}}
static int fuseb_destroy(backend_t *backend){ // {{{
	//fuse_userdata      *userdata = (fuse_userdata *)backend->userdata;
	//free(userdata);
	return 0;
} // }}}
static int fuseb_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	char                  *hk_backend_str    = "backend";
	//fuse_userdata         *userdata          = (fuse_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, hk_backend_str, config, HK(input));
	
	vfs_item_create("/backends", VFS_FOLDER, NULL);
	if(vfs_item_create("",              VFS_FOLDER, backend) == NULL)
		return -1;
	if(vfs_item_create("/whole",        VFS_FILE,   backend) == NULL)
		return -1;
	/*
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	
	if(fuse_opt_parse(&args, &opts, frozenfs_opts, frozenfs_opt_proc) == -1)
		exit(1);
	
	if(!opts.datadir){
		fprintf(stderr, "missing datadir");
		exit(1);
	}
	
	ret = fuse_main(args.argc, args.argv, &fuse_operations, NULL);*/
	return 0;
} // }}}

backend_t fuse_proto = {
	.class          = "io/fuse",
	.supported_api  = API_HASH,
	.func_init      = &fuseb_init,
	.func_destroy   = &fuseb_destroy,
	.func_configure = &fuseb_configure
};

