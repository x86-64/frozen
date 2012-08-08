#define FUSE_USE_VERSION 26
#define _GNU_SOURCE

#include <libfrozen.h>
#include <fuse.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_fuse io/fuse
 */
/**
 * @ingroup mod_machine_fuse
 * @page page_fuse_info Description
 *
 * This machine add fuse filesystem support for frozen. You can use it as input and output for any machine.
 */
/**
 * @ingroup mod_machine_fuse
 * @page page_fuse_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "io/fuse",
 *              mountpoint              = "/mnt/somepoint/",  # mountpoint for fuse fs
 *              multithread             = (uint_t)'1',        # enable multithreaded fuse operations, default 0 // NOTE don't use it until all machines would be multithread-capable
 *              items                   = {                   # define files and folders to create in fs
 *                   {                                        # - first example item
 *                       path           = "/onefile",         #     full relative path to file/folder starting with /
 *                       folder         = (uint_t)'1',        #     is it folder?, default 0
 *                       machine        = (machine_t)"bname"  #     machine for request sending
 *                   },
 *                   ...
 *              }
 * }
 * @endcode
 */

#define ERRORS_MODULE_ID 28
#define ERRORS_MODULE_NAME "io/fuse"

typedef struct fuseb_userdata {
	char                  *mountpoint;
	hash_t                *items;
} fuseb_userdata;

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
	
	void                  *userdata;
	
	// fs related
	struct stat            stat;
};

typedef struct vfs_root_userdata {
	struct fuse           *fuse;
	struct fuse_chan      *ch;
	char                  *mountpoint;
	pthread_t              fs_thread;
	uintmax_t              fs_thread_ok;
} vfs_root_userdata;

static list                    vfs_catalog       = LIST_INITIALIZER;
struct fuse_operations         fuse_operations;

/* vfs {{{ */
static int        match_path(char *folderpath, char *filepath){ // {{{
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
} // }}}

static vfs_item * vfs_item_find(vfs_item *root, char *path, size_t path_len){ // {{{
	vfs_item    *folder;
	
	if(strcmp(path, "") == 0 || strcmp(path, "/") == 0)
		return root;
	
	//printf("find: %s\n", path);
	for(folder = root; folder != NULL;){
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
} // }}}
static vfs_item * vfs_item_find_parent(vfs_item *root, char *path){ // {{{
	size_t                 path_len;
	char                  *name;
	vfs_item              *folder;
	
	if( (name = strrchr(path, '/')) == NULL)
		return NULL;
	
	path_len = name - path;
	
	if( (folder = vfs_item_find(root, path, path_len)) == NULL)
		return NULL;
	
	return folder;
} // }}}
static void       vfs_item_destroy(vfs_item *root, vfs_item *item){ // {{{
	vfs_item              *folder, *child;
	
	folder = vfs_item_find_parent(root, item->path);

	// unlink
	if(folder->childs != item){
		for(child = folder->childs; child != NULL && child->next != item; child = child->next);
		child->next = item->next;
	}else{
		folder->childs = item->next;
	}
	
	// destroy childs
	while(item->childs != NULL)
		vfs_item_destroy(root, item->childs);
	
	pipeline_destroy((machine_t *)item->userdata);

	free(item->path);
	free(item);
} // }}}
static vfs_item * vfs_item_create(vfs_item *root, char *path, vfs_item_type vfs_type, machine_t *machine){ // {{{
	vfs_item              *folder, *new_item;
	
	if( (folder = vfs_item_find_parent(root, path)) == NULL)
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
	
	new_item->userdata = (void *)machine;
	
	// link it
	new_item->next = folder->childs;
	folder->childs = new_item;
	return new_item;
} // }}}
static vfs_item * vfs_find(char *mountpoint){ // {{{
	vfs_item              *vfs;
	vfs_root_userdata     *vfs_userdata;
	void                  *iter              = NULL;
	
	list_rdlock(&vfs_catalog);
		while( (vfs = list_iter_next(&vfs_catalog, &iter)) != NULL){
			vfs_userdata = (vfs_root_userdata *)vfs->userdata;
			
			if(strcasecmp(mountpoint, vfs_userdata->mountpoint) == 0)
				goto found;
		}
		vfs = NULL;
found:
	list_unlock(&vfs_catalog);
	return vfs;
} // }}}
static void       vfs_destroy(vfs_item *vfs){ // {{{
	void                  *res;
	vfs_root_userdata     *vfs_userdata;
	
	if(vfs != NULL){
		list_delete(&vfs_catalog, vfs);

		vfs_userdata = (vfs_root_userdata *)vfs->userdata;
		if(vfs_userdata != NULL){
			if(vfs_userdata->fs_thread_ok == 1){
				fuse_exit(vfs_userdata->fuse);
				
				// remove thread (remove this?)
				pthread_cancel(vfs_userdata->fs_thread);
				pthread_join(vfs_userdata->fs_thread, &res);
			}
			
			if(vfs_userdata->ch)
				fuse_unmount(vfs_userdata->mountpoint, vfs_userdata->ch);
			if(vfs_userdata->fuse)
				fuse_destroy(vfs_userdata->fuse);
			if(vfs_userdata->mountpoint)
				free(vfs_userdata->mountpoint);
			
			free(vfs_userdata);
		}
		
		// TODO recursive remove childs
		
		if(vfs->path)
			free(vfs->path);
		free(vfs);
	}
} // }}}
static vfs_item * vfs_create(char *mountpoint, uintmax_t multithreaded){ // {{{
	vfs_item              *vfs; 
	vfs_root_userdata     *vfs_userdata;
	
	if( (vfs = calloc(1, sizeof(vfs_item))) == NULL)
		goto err;
	if( (vfs_userdata = calloc(1, sizeof(vfs_root_userdata))) == NULL)
		goto err;
	
	vfs->name          = vfs->path + 1;
	vfs->type          = VFS_FOLDER;
	vfs->stat.st_mode  = S_IFDIR | 0755;
	vfs->stat.st_size  = 0;
	vfs->stat.st_nlink = 1;
	vfs->userdata      = vfs_userdata;
	
	if( (vfs->path = strdup("/")) == NULL)
		goto err;
	if( (vfs_userdata->mountpoint = strdup(mountpoint)) == NULL)
		goto err;
	if( (vfs_userdata->ch = fuse_mount(mountpoint, NULL)) == NULL )
		goto err;
	if( (vfs_userdata->fuse = fuse_new(vfs_userdata->ch, NULL, &fuse_operations, sizeof(fuse_operations), (void *)vfs)) == NULL)
		goto err;
	
	if(multithreaded){
		if(pthread_create(&vfs_userdata->fs_thread, NULL, (void *(*)(void *))&fuse_loop_mt, vfs_userdata->fuse) != 0)
			goto err;
	}else{
		if(pthread_create(&vfs_userdata->fs_thread, NULL, (void *(*)(void *))&fuse_loop,    vfs_userdata->fuse) != 0)
			goto err;
	}
	vfs_userdata->fs_thread_ok = 1;
	
	list_add(&vfs_catalog, vfs);
	return vfs;	
err:
	vfs_destroy(vfs);
	return NULL;
} // }}}
/* }}} */
/* folders {{{ */
static int fuseh_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi){
	vfs_item              *folder, *child;
	struct fuse_context   *fctx              = fuse_get_context();
	vfs_item              *root              = (vfs_item *)fctx->private_data;
	
	if( (folder = vfs_item_find(root, (char *)path, ~0)) == NULL)
		return errorn(ENOENT);
	
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
	struct fuse_context   *fctx              = fuse_get_context();
	vfs_item              *root              = (vfs_item *)fctx->private_data;
	
	if( (item = vfs_item_find(root, (char *)path, ~0)) == NULL)
		return errorn(ENOENT);
	
	if(item->type == VFS_FILE){
		// updating size
		request_t r_count[] = {
			{ HK(action),  DATA_ACTIONT(ACTION_LENGTH)       },
			{ HK(buffer),  DATA_OFFT(0)                      },
			hash_end
		};
		machine_query((machine_t *)item->userdata, r_count);
		size = DEREF_TYPE_OFFT(&r_count[1].data);
	}
	item->stat.st_size = size; //(param) ?
	//	((offset > size) ? 0 : size - offset) :
	//	size;
	
	memcpy(buf, &(item->stat), sizeof(struct stat));
	return 0;
}
/* }}} */
/* files {{{ */ 
static int fuseh_open(const char *path, struct fuse_file_info *fi){ // {{{
	vfs_item              *item;
	struct fuse_context   *fctx              = fuse_get_context();
	vfs_item              *root              = (vfs_item *)fctx->private_data;
	
	if( (item = vfs_item_find(root, (char *)path, ~0)) == NULL)
		return errorn(ENOENT);
	
	// TODO check access
	// TODO lock item
	
	fi->fh = (unsigned long)item;
	return 0;
} // }}}
static int fuseh_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi){ // {{{
	ssize_t                ret, ret2;
	vfs_item              *item              = (vfs_item *)(uintptr_t)fi->fh;
	
	request_t r_read[] = {
		{ HK(action), DATA_ACTIONT(ACTION_READ)                  },
		{ HK(offset), DATA_OFFT(off)                             },
		{ HK(buffer), DATA_RAW(buf, size)                        },
		{ HK(path),   DATA_PTR_STRING((char *)path)              },
		{ HK(ret),    DATA_SIZET(0)                              },
		hash_end
	};
	ret = machine_query((machine_t *)item->userdata, r_read);
	ret2 = DEREF_TYPE_SIZET(&r_read[4].data);

	//printf("read: %x bytes from %x. ret: %x, data: %s\n", (int)size, (int)off, (int)ret, buf);
	
	return (ret < 0) ? 0 : (int)ret2; //(int)size;
} // }}}
static int fuseh_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi){ // {{{
	ssize_t                ret;
	vfs_item              *item              = (vfs_item *)(uintptr_t)fi->fh;
	
	request_t r_write[] = {
		{ HK(action),  DATA_ACTIONT(ACTION_WRITE)                      },
		{ HK(offset),  DATA_OFFT(off)                             },
		{ HK(buffer),  DATA_RAW((char *)buf, size)                     },
		{ HK(path),    DATA_PTR_STRING((char *)path)                   },
		hash_end
	};
	ret = machine_query((machine_t *)item->userdata, r_write);
	
	//printf("write: %x bytes from %x. ret: %x\n", (int)size, (int)off, (int)ret);
	
	return (ret < 0) ? 0 : size;
} // }}}
static int fuseh_release(const char *path, struct fuse_file_info *fi){ // {{{
	// TODO unlock item
	return 0;
} // }}}
/* }}} */
/* stubs {{{
static int fuseh_flush(const char *path, struct fuse_file_info *fi){
	return errorn(ENOSYS);
}

static int fuseh_statfs(const char *path, struct statvfs *stat){
	return 0;
}

static int fuseh_readlink(const char *path, char *buf, size_t size){
	return errorn(ENOSYS);
}

static int fuseh_mknod(const char *path, mode_t mode, dev_t dev){
	return errorn(ENOSYS);
}

static int fuseh_mkdir(const char *path, mode_t mode){
	return errorn(ENOSYS);
}

static int fuseh_unlink(const char *path){
	return errorn(ENOSYS);
}

static int fuseh_rmdir(const char *path){
	return errorn(ENOSYS);
}

static int fuseh_symlink(const char *from, const char *to){
	return errorn(ENOSYS);
}

static int fuseh_rename(const char *from, const char *to){
	return errorn(ENOSYS);
}

static int fuseh_link(const char *from, const char *to){
	return errorn(ENOSYS);
}

static int fuseh_chmod(const char *path, mode_t mode){
	return errorn(ENOSYS);
}

static int fuseh_chown(const char *path, uid_t uid, gid_t gid){
	return errorn(ENOSYS);
}

static int fuseh_truncate(const char *path, off_t off){
	return errorn(ENOSYS);
}

static int fuseh_fsync(const char *path, int datasync, struct fuse_file_info *fi){
	return errorn(ENOSYS);
}

static int fuseh_setxattr(const char *path, const char *name, const char *value, size_t size, int pos){
	return errorn(ENOSYS);
}

static int fuseh_getxattr(const char *path, const char *name, char *value, size_t size){
	return errorn(ENOSYS);
}

static int fuseh_listxattr(const char *path, char *list, size_t size){
	return errorn(ENOSYS);
}

static int fuseh_removexattr(const char *path, const char *name){
	return errorn(ENOSYS);
}

static int fuseh_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi){
	return errorn(ENOSYS);
}

static int fuseh_access(const char *path, int mask){
	return errorn(ENOSYS);
}

static int fuseh_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	return errorn(ENOSYS);
}

static int fuseh_ftruncate(const char *path, off_t off, struct fuse_file_info *fi){
	return errorn(ENOSYS);
}

static int fuseh_fgetattr(const char *path, struct stat *buf, struct fuse_file_info *fi){
	return errorn(ENOSYS);
}

static int fuseh_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *lock){
	return errorn(ENOSYS);
}

static int fuseh_utimens(const char *path, const struct timespec tv[2]){
	return errorn(ENOSYS);
}

static int fuseh_bmap(const char *path, size_t blocksize, uint64_t *idx){
	return errorn(ENOSYS);
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

static ssize_t fuseb_item_configure(hash_t *hash, vfs_item *root){ // {{{
	ssize_t                ret;
	data_t                *data;
	hash_t                *item_config;
	uintmax_t              item_type         = 0;       // 0 = file, 1 = folder
	char                  *item_path         = NULL;
	machine_t             *item_machine      = NULL;
	
	data = &hash->data;
	if( data->type == TYPE_HASHT ){
		item_config = (hash_t *)(data->ptr);
		
		hash_data_convert(ret, TYPE_STRINGT,  item_path,    item_config, HK(path));   if(ret != 0) return ITER_BREAK;
		hash_data_get    (ret, TYPE_UINTT,    item_type,    item_config, HK(folder));
		hash_data_consume(ret, TYPE_MACHINET, item_machine, item_config, HK(machine));
		
		vfs_item_create(
			root,
			item_path,
			(item_type == 0) ? VFS_FILE : VFS_FOLDER,
			item_machine
		);

		free(item_path);
	}
	return ITER_CONTINUE;
} // }}}
static ssize_t fuseb_item_destroy(hash_t *hash, vfs_item *root){ // {{{
	ssize_t                ret;
	data_t                *data;
	vfs_item              *vfs_item;
	hash_t                *item_config;
	char                  *item_path         = NULL;
	
	data = &hash->data;
	if( data->type == TYPE_HASHT ){
		item_config = (hash_t *)(data->ptr);
		
		hash_data_convert(ret, TYPE_STRINGT,  item_path,    item_config, HK(path));
		
		if( (vfs_item = vfs_item_find(root, item_path, ~0)) == NULL){
			free(item_path);
			return ITER_CONTINUE;
		}
		
		vfs_item_destroy(root, vfs_item);
		free(item_path);
	}
	return ITER_CONTINUE;
} // }}}

static ssize_t fuseb_init(machine_t *machine){ // {{{
	ssize_t                ret               = 0;
	fuseb_userdata        *userdata          = machine->userdata = calloc(1, sizeof(fuseb_userdata));
	if(userdata == NULL)
		return errorn(ENOMEM);
	
	return ret;
} // }}}
static ssize_t fuseb_destroy(machine_t *machine){ // {{{
	vfs_item              *root;
	fuseb_userdata        *userdata          = machine->userdata;
	
	if( (root = vfs_find(userdata->mountpoint)) == NULL)
		return 0;
	
	hash_iter(userdata->items, (hash_iterator)&fuseb_item_destroy, (void *)root, 0);
	
	if(root->childs == NULL)
		vfs_destroy(root);
	
	hash_free(userdata->items);
	free(userdata->mountpoint);
	free(userdata);
	return 0;
} // }}}
static ssize_t fuseb_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	vfs_item              *root;
	uintmax_t              multithreaded     = 0;
	fuseb_userdata        *userdata          = machine->userdata;
	
	hash_data_get    (ret, TYPE_UINTT,   multithreaded,        config, HK(multithread));
	hash_data_convert(ret, TYPE_STRINGT, userdata->mountpoint, config, HK(mountpoint));
	hash_data_consume(ret, TYPE_HASHT,   userdata->items,      config, HK(items));
	
	if(userdata->mountpoint == NULL)
		return error("mountpoint not supplied");
	
	if( (root = vfs_find(userdata->mountpoint)) == NULL)
		if( (root = vfs_create(userdata->mountpoint, multithreaded)) == NULL)
			return error("fuse fs creation failed");
	
	if(hash_iter(userdata->items, (hash_iterator)&fuseb_item_configure, (void *)root, 0) != ITER_OK)
		return error("fuse items confguration failed");
	return 0;
} // }}}

machine_t fuse_proto = {
	.class          = "io/fuse",
	.supported_api  = API_HASH,
	.func_init      = &fuseb_init,
	.func_configure = &fuseb_configure,
	.func_destroy   = &fuseb_destroy
};

