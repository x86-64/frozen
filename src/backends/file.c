#include <libfrozen.h>

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
	
	// TODO add realpath and checks
	
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

static ssize_t file_create(chain_t *chain, request_t *request, buffer_t *buffer){
	int               fd;
	int               ret;
	off_t             new_key;
	pthread_mutex_t  *mutex;
	unsigned int      value_size;
	
	if(hash_get_copy(request, "size", TYPE_INT32, &value_size, sizeof(value_size)) != 0)
		return -EINVAL;
	
	fd    =  ((file_user_data *)chain->user_data)->handle;
	mutex = &((file_user_data *)chain->user_data)->create_lock;
	
	pthread_mutex_lock(mutex);
		
		new_key = lseek(fd, 0, SEEK_END);
		ret = ftruncate(fd, new_key + value_size);
		if(ret == -1){
			return -errno;
		}
		
		buffer_write(buffer, sizeof(off_t), *(off_t *)chunk = new_key); 
		
	pthread_mutex_unlock(mutex);
	
	return sizeof(off_t);
}

static ssize_t file_set(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t           ret;
	
	off_t             key;
	data_t           *value;
	size_t            value_size;
	
	if(hash_get_copy (request, "key",   TYPE_INT64,  &key,   sizeof(key)) != 0)
		return -EINVAL;
	if(hash_get      (request, "value", TYPE_BINARY, &value, &value_size) != 0) 
		return -EINVAL;
	
redo:
	ret = pwrite(
		((file_user_data *)chain->user_data)->handle,
		value,
		value_size,
		key
	);
	if(ret == -1 || ret != value_size){
		if(errno == EINTR) goto redo;
		return -errno;
	}
	
	return ret;
}

static ssize_t file_get(chain_t *chain, request_t *request, buffer_t *buffer){
	int               fd;
	ssize_t           ret;
	off_t             key;
	unsigned int      value_size;
	
	if(hash_get_copy (request, "key",   TYPE_INT64, &key,        sizeof(key)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "size",  TYPE_INT32, &value_size, sizeof(value_size)) != 0)
		return -EINVAL;
	
	fd = ((file_user_data *)chain->user_data)->handle;
	
redo:
	buffer_write(buffer, value_size,
		do{
			ret = pread(fd, chunk, size, key + offset);
			if(ret == -1 || ret != value_size){
				if(errno == EINTR) goto redo;
				return -errno;
			}
		}while(0)
	);
	
	return value_size;
}

static ssize_t file_delete(chain_t *chain, request_t *request, buffer_t *buffer){
	return 0;
}

static ssize_t file_move(chain_t *chain, request_t *request, buffer_t *buffer){
	off_t         from, to;
	size_t        ssize, size, max_size;
	unsigned int  value_size;
	char *        buff;
	int           direction;
	int           ret = 0;
	
	if(hash_get_copy (request, "key_from",  TYPE_INT64, &from,       sizeof(from)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "key_to",    TYPE_INT64, &to,         sizeof(to)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "size",      TYPE_INT32, &value_size, sizeof(value_size)) != 0)
		return -EINVAL;
	
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
		return -ENOMEM;
	
	while(value_size > 0){
		if( (ssize = pread(data->handle, buff, size, from)) == -1){
			if(errno == EINTR) continue;
			ret = -errno;
			break;
		}
		if(ssize == 0){
			ret = -EINVAL;
			break;
		}
		
		if( (ssize = pwrite(data->handle, buff, ssize, to) == -1)){
			if(errno == EINTR) continue;
			ret = -errno;
			break;
		}
		if(ssize == 0){
			// TODO what the hell, pwrite returns 0 in any case
			//ret = -EINVAL;
			//break;
		}
		
		if(direction == 1){ from += size; }else{ from -= size; }
		if(direction == 1){ to   += size; }else{ to   -= size; }
		value_size -= size;
		size        = (max_size < value_size) ? max_size : value_size;
	
	}
	free(buff);
	return ret;
}

static ssize_t file_count(chain_t *chain, request_t *request, buffer_t *buffer){
	struct stat  buf;
	int          ret;
	
	ret = fstat(
		((file_user_data *)chain->user_data)->handle,
		&buf
	);
	if(ret == 0){
		buffer_write(buffer, sizeof(buf.st_size),
			memcpy(chunk, &(buf.st_size), sizeof(buf.st_size))
		);
	}
	
	return sizeof(buf.st_size);
}

static chain_t chain_file = {
	"file",
	CHAIN_TYPE_CRWD,
	&file_init,
	&file_configure,
	&file_destroy,
	{{
		&file_create,
		&file_set,
		&file_get,
		&file_delete,
		&file_move,
		&file_count
	}}
};
CHAIN_REGISTER(chain_file)

