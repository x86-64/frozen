#include <libfrozen.h>

typedef enum file_stat_status {
	STAT_NEED_UPDATE,
	STAT_UPDATED,
	STAT_ERROR
} file_stat_status;

typedef struct file_user_data {
	char *           path;
	int              handle;
	size_t           copy_buffer;
	struct stat      file_stat;
	file_stat_status file_stat_status;
	
	pthread_mutex_t  create_lock;
	
} file_user_data;

static file_stat_status  file_update_count(file_user_data *data){ // {{{
	int              ret;
	
	switch(data->file_stat_status){
		case STAT_NEED_UPDATE:
		case STAT_ERROR:
			ret = fstat(
		 		data->handle,
				&data->file_stat
			);
			data->file_stat_status = (ret == 0) ? STAT_UPDATED : STAT_ERROR;
			break;
		case STAT_UPDATED:
			break;
	};
	return data->file_stat_status;
} // }}}

static int file_init(chain_t *chain){
	file_user_data *user_data = calloc(1, sizeof(file_user_data));
	if(user_data == NULL)
		return -ENOMEM;
	
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
	if(filename == NULL)
		return -EINVAL;
	
	//	filename = setting_get_child_string(config, "backend_name");
	//	if(filename == NULL)
	//		goto cleanup;
	//}
	
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
	
	data->path             = filepath;
	data->handle           = handle;
	data->copy_buffer      = copy_buffer_size;
	data->file_stat_status = STAT_NEED_UPDATE;
	
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
	file_user_data   *data        = ((file_user_data *)chain->user_data);
	
	if(hash_get_copy(request, "size", TYPE_INT32, &value_size, sizeof(value_size)) != 0)
		return -EINVAL;
	
	fd    =  data->handle;
	mutex = &data->create_lock;
	
	pthread_mutex_lock(mutex);
		
		new_key = lseek(fd, 0, SEEK_END);
		ret = ftruncate(fd, new_key + value_size);
		if(ret == -1){
			return -errno;
		}
		
		buffer_write(buffer, sizeof(off_t), *(off_t *)chunk = new_key); 
		
		data->file_stat_status = STAT_NEED_UPDATE;
		
	pthread_mutex_unlock(mutex);
	
	return sizeof(off_t);
}

static ssize_t file_set(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t           ret;
	
	off_t             key;
	data_t           *value;
	data_t           *value_data;
	size_t            value_size;
	size_t            write_size;
	file_user_data   *data        = ((file_user_data *)chain->user_data);
	
	if(hash_get_copy (request, "key",   TYPE_INT64,  &key,        sizeof(key)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "size",  TYPE_INT32,  &write_size, sizeof(write_size)) != 0)
		return -EINVAL;
	if(hash_get      (request, "value", TYPE_BINARY, &value,      &value_size) != 0) 
		return -EINVAL;
	
	value_data = data_binary_ptr(value);
	value_size = data_binary_len(value);
	
	if(write_size > value_size) // requested write more than have in buffer
		write_size = value_size;
	
redo:
	ret = pwrite(
		((file_user_data *)chain->user_data)->handle,
		value_data,
		write_size,
		key
	);
	if(ret == -1 || ret != write_size){
		if(errno == EINTR) goto redo;
		return -errno;
	}
	
	data->file_stat_status = STAT_NEED_UPDATE; // coz can write to end, without calling create
	
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
			if(ret == -1 || ret != size){
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
	off_t            from, to;
	unsigned int     req_size;
	
	size_t           move_size;
	size_t           ssize, size, max_size;
	char *           buff;
	int              direction;
	int              ret         = 0;
	file_user_data  *data        = ((file_user_data *)chain->user_data);
	
	if(hash_get_copy (request, "key_from",  TYPE_INT64, &from,       sizeof(from)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "key_to",    TYPE_INT64, &to,         sizeof(to)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "size",      TYPE_INT32, &req_size,   sizeof(req_size)) != 0){
		if(file_update_count(data) == STAT_ERROR)
			return -EINVAL;
		
		move_size = (data->file_stat.st_size - from);
	}else{
		move_size = req_size;
	}
	
	if(from == to || move_size == 0)
		return -EINVAL;
	
	if(from < to){
		max_size   = ((to - from) > data->copy_buffer) ? data->copy_buffer : (size_t)(to - from);
		size       = (max_size < move_size)            ? max_size          : move_size;
		
		from      += move_size - size;
		to        += move_size - size;
		direction  = -1;
	}else{
		max_size   = ((from - to) > data->copy_buffer) ? data->copy_buffer : (size_t)(from - to);
		size       = (max_size < move_size)            ? max_size          : move_size;
			
		direction  = 1;
	}
	
	if(size == 0)
		return -EINVAL;
	
	buff = (char *)malloc(size);
	if(buff == NULL)
		return -ENOMEM;
	
	while(move_size > 0){
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
		move_size  -= size;
		size        = (max_size < move_size) ? max_size : move_size;
	}
	free(buff);
	
	data->file_stat_status = STAT_NEED_UPDATE;
	
	return ret;
}

static ssize_t file_count(chain_t *chain, request_t *request, buffer_t *buffer){
	file_user_data  *data = ((file_user_data *)chain->user_data);
	
	if(file_update_count(data) == STAT_ERROR)
		return -EINVAL;
	
	buffer_write(buffer, sizeof(data->file_stat.st_size),
		memcpy(chunk, &(data->file_stat.st_size), sizeof(data->file_stat.st_size))
	);
	
	return sizeof(data->file_stat.st_size);
}

static chain_t chain_file = {
	"file",
	CHAIN_TYPE_CRWD,
	&file_init,
	&file_configure,
	&file_destroy,
	{{
		.func_create = &file_create,
		.func_set    = &file_set,
		.func_get    = &file_get,
		.func_delete = &file_delete,
		.func_move   = &file_move,
		.func_count  = &file_count
	}}
};
CHAIN_REGISTER(chain_file)

