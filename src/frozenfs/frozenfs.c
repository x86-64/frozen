#include <fuse.h>
#include <errno.h>
#include <fcntl.h>

static int fuse_getattr(const char *path, struct stat *buf){
	return -ENOSYS;
}

static int fuse_readlink(const char *path, char *buf, size_t size){
	return -ENOSYS;
}

static int fuse_mknod(const char *path, mode_t mode, dev_t dev){
	return -ENOSYS;
}

static int fuse_mkdir(const char *path, mode_t mode){
	return -ENOSYS;
}

static int fuse_unlink(const char *path){
	return -ENOSYS;
}

static int fuse_rmdir(const char *path){
	return -ENOSYS;
}

static int fuse_symlink(const char *from, const char *to){
	return -ENOSYS;
}

static int fuse_rename(const char *from, const char *to){
	return -ENOSYS;
}

static int fuse_link(const char *from, const char *to){
	return -ENOSYS;
}

static int fuse_chmod(const char *path, mode_t mode){
	return -ENOSYS;
}

static int fuse_chown(const char *path, uid_t uid, gid_t gid){
	return -ENOSYS;
}

static int fuse_truncate(const char *path, off_t off){
	return -ENOSYS;
}

static int fuse_open(const char *path, struct fuse_file_info *fi){
	return 0;
}

static int fuse_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuse_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuse_statfs(const char *path, struct statvfs *stat){
	return 0;
}

static int fuse_flush(const char *path, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuse_release(const char *path, struct fuse_file_info *fi){
	return 0;
}

static int fuse_fsync(const char *path, int datasync, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuse_setxattr(const char *path, const char *name, const char *value, size_t size, int pos){
	return -ENOSYS;
}

static int fuse_getxattr(const char *path, const char *name, char *value, size_t size){
	return -ENOSYS;
}

static int fuse_listxattr(const char *path, char *list, size_t size){
	return -ENOSYS;
}

static int fuse_removexattr(const char *path, const char *name){
	return -ENOSYS;
}

static int fuse_opendir(const char *path, struct fuse_file_info *fi){
	return 0;
}

static int fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t off, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuse_releasedir(const char *path, struct fuse_file_info *fi){
	return 0;
}

static int fuse_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuse_access(const char *path, int mask){
	return -ENOSYS;
}

static int fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuse_ftruncate(const char *path, off_t off, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuse_fgetattr(const char *path, struct stat *buf, struct fuse_file_info *fi){
	return -ENOSYS;
}

static int fuse_lock(const char *path, struct fuse_file_info *fi, int cmd, struct flock *lock){
	return -ENOSYS;
}

static int fuse_utimens(const char *path, const struct timespec tv[2]){
	return -ENOSYS;
}

static int fuse_bmap(const char *path, size_t blocksize, uint64_t *idx){
	return -ENOSYS;
}

static int fuse_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi, unsigned int flags, void *data){
	return -ENOSYS;
}

static int fuse_poll(const char *path, struct fuse_file_info *fi, struct fuse_pollhandle *ph, unsigned *reventsp){
	return -ENOSYS;
}

static void* fuse_init(struct fuse_conn_info *conn){
	return NULL;
}
static void fuse_destroy(void *userdata){
	return;
}

struct fuse_operations fuse_operations = {
	.getattr = fuse_getattr,
	.readlink = fuse_readlink,
	.mknod = fuse_mknod,
	.mkdir = fuse_mkdir,
	.unlink = fuse_unlink,
	.rmdir = fuse_rmdir,
	.symlink = fuse_symlink,
	.rename = fuse_rename,
	.link = fuse_link,
	.chmod = fuse_chmod,
	.chown = fuse_chown,
	.truncate = fuse_truncate,
	.open = fuse_open,
	.read = fuse_read,
	.write = fuse_write,
	.statfs = fuse_statfs,
	.flush = fuse_flush,
	.release = fuse_release,
	.fsync = fuse_fsync,
	.setxattr = fuse_setxattr,
	.getxattr = fuse_getxattr,
	.listxattr = fuse_listxattr,
	.removexattr = fuse_removexattr,
	.opendir = fuse_opendir,
	.readdir = fuse_readdir,
	.releasedir = fuse_releasedir,
	.fsyncdir = fuse_fsyncdir,
	.init = fuse_init,
	.destroy = fuse_destroy,
	.access = fuse_access,
	.create = fuse_create,
	.ftruncate = fuse_ftruncate,
	.fgetattr = fuse_fgetattr,
	.lock = fuse_lock,
	.utimens = fuse_utimens,
	.bmap = fuse_bmap,
	.ioctl = fuse_ioctl,
	.poll = fuse_poll
};

int main(int argc, char *argv[]){
	return fuse_main(argc, argv, &fuse_operations, 0);
}

