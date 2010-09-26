#include <libfrozen.h>
#include <backend.h>

// TODO read\write error handling
// TODO crazy logic in file_move, can rewrite?

typedef struct file_user_data {
	char *           path;
	int              handle;
	size_t           copy_buffer;
	
	pthread_mutex_t  create_lock;
	
} file_user_data;

static int file_init(chain_t *chain){
	file_user_data *user_data = malloc(sizeof(file_user_data));
	if(user_data == NULL)
		return -ENOMEM;
	
	memset(user_data, 0, sizeof(file_user_data));
	
	chain->user_data = user_data;
	
	return 0;
}

static int file_destroy(chain_t *chain){
	file_user_data *data = (file_user_data *)chain->user_data;
	
	if(data->path){
		// was inited
		// TODO rewrite condition

		free(data->path);
		close(data->handle);
		pthread_mutex_destroy(&(data->create_lock));
	}
	
	free(chain->user_data);
	
	chain->user_data = NULL;
	
	return 0;
}

static int file_configure(chain_t *chain, setting_t *config){
	char   *filepath;
	char   *filename;
	char   *temp;
	int     handle;
	size_t  copy_buffer_size;
	
	filepath  = (char *)malloc(256);
	*filepath = '\0';
	
	filename = setting_get_child_string(config, "filename");
	if(filename == NULL){
		filename = setting_get_child_string(config, "backend_name");
		if(filename == NULL)
			goto cleanup;
	}
	
	temp = setting_get_child_string(global_settings, "homedir");
	if(temp == NULL)
		goto cleanup;
	
	
	if(snprintf(filepath, 256, "%s/%s.dat", temp, filename) >= 256){
		/* snprintf can truncate strings, so malicious user can use it to access another file.
		 * we check string len such way to ensure buffer not filled and none of strings was truncated */
		goto cleanup;
	}
	
	handle = open(
		filepath,
		O_CREAT | O_RDWR | O_LARGEFILE,
		S_IRUSR | S_IWUSR
	);
	
	if(handle == -1){
		goto cleanup;
	}
	
	temp = setting_get_child_string(config, "copy_buffer_size");
	copy_buffer_size = (temp != NULL) ? strtoul(temp, NULL, 10) : 512;
	
	file_user_data *data = (file_user_data *)chain->user_data;
	
	data->path        = filepath;
	data->handle      = handle;
	data->copy_buffer = copy_buffer_size;
	
	pthread_mutex_init(&(data->create_lock), NULL);
	
	return 0;

cleanup:
	free(filepath);
	return -EINVAL;
}

static int file_create(chain_t *chain, void *key, size_t value_size){
	int               fd;
	int               ret;
	off_t             new_key;
	pthread_mutex_t  *mutex;
	

	fd    =  ((file_user_data *)chain->user_data)->handle;
	mutex = &((file_user_data *)chain->user_data)->create_lock;
	
	pthread_mutex_lock(mutex);
		
		new_key = lseek(fd, 0, SEEK_END);
		ret = ftruncate(fd, new_key + value_size);
		if(ret == -1){
			return -errno;
		}
		
		*(off_t *)key = new_key;
		
	pthread_mutex_unlock(mutex);
	
	return 0;
}

static int file_set(chain_t *chain, void *key, buffer_t *value, size_t value_size){
	int      fd;
	ssize_t  ret;
	void    *chunk;

	fd = ((file_user_data *)chain->user_data)->handle;
	
	chunk = buffer_get_first_chunk(value);
	// TODO length check
	// TODO writing chain of chunks

redo:
	ret = pwrite(fd, chunk, value_size, *(off_t *)key);
	if(ret == -1 || ret != value_size){
		if(errno == EINTR) goto redo;
		return -errno;
	}
	
	return 0;
}

static int file_get(chain_t *chain, void *key, buffer_t *value, size_t value_size){
	int      fd;
	ssize_t  ret;
	void    *chunk;
	
	fd = ((file_user_data *)chain->user_data)->handle;
	
	chunk = chunk_alloc(value_size);
	buffer_add_tail_chunk(value, chunk);
	
redo:
	ret = pread(fd, chunk, value_size, *(off_t *)key);
	if(ret == -1 || ret != value_size){
		if(errno == EINTR) goto redo;
		return -errno;
	}
	
	return 0;
}

static int file_delete(chain_t *chain, void *key, size_t value_size){
	return 0;
}

static int file_move(chain_t *chain, void *key_from, void *key_to, size_t value_size){
	off_t  from = *(off_t *)key_from;
	off_t  to   = *(off_t *)key_to;
	off_t  curr_from, curr_to;
	size_t ssize, size, max_size;
	char * buff;
	int    direction;
	int    ret = 0;
	
	if(from == to || value_size == 0)
		return -EINVAL;
	
	file_user_data * data = ((file_user_data *)chain->user_data);
	
	if(from < to){
		max_size   = ((to - from) > data->copy_buffer) ? data->copy_buffer : (size_t)(to - from);
		size       = (max_size < value_size)           ? max_size          : value_size;
		
		from      += value_size - size;
		to        += value_size - size;
		direction  = -1;
	}else{
		max_size   = ((from - to) > data->copy_buffer) ? data->copy_buffer : (size_t)(from - to);
		size       = (max_size < value_size)           ? max_size          : value_size;
			
		direction  = 1;
	}
	
	if(size == 0)
		return -EINVAL;
	
	buff = (char *)malloc(size);
	if(buff == NULL)
		return -EINVAL;
	
	while(value_size > 0){
		ssize = pread(data->handle, buff, size, from);
		if(ssize == -1){
			if(errno == EINTR) continue;
			ret = -errno;
			break;
		}
		
		ssize = pwrite(data->handle, buff, ssize, to);
		if(ret == -1){
			if(errno == EINTR) continue;
			ret = -errno;
			break;
		}
		
		value_size -= ssize;
		from       += ssize * direction;
		to         += ssize * direction;
		size        = (max_size < value_size) ? max_size : value_size;
	}
	
	free(buff);
	return ret;
}

static int file_count(chain_t *chain, void *count){
	struct stat  buf;
	int          ret;
	int          fd;
	
	fd = ((file_user_data *)chain->user_data)->handle;
	
	ret = fstat(fd, &buf);
	if(ret == 0){
		memcpy(count, &(buf.st_size), sizeof(buf.st_size));
	}
	
	return ret;
}

static chain_t chain_file = {
	"file",
	CHAIN_TYPE_CRWD,
	&file_init,
	&file_configure,
	&file_destroy,
	{
		&file_create,
		&file_set,
		&file_get,
		&file_delete,
		&file_move,
		&file_count
	}
};
CHAIN_REGISTER(chain_file)

