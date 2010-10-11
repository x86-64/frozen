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
	
	filename = setting_get_child_string(config, "filename");
	if(filename == NULL)
		return -EINVAL;
	
	//	filename = setting_get_child_string(config, "backend_name");
	//	if(filename == NULL)
	//		goto cleanup;
	//}
	
	filepath  = malloc(256);
	*filepath = '\0';
	
	temp = setting_get_child_string(global_settings, "homedir");
	if(temp == NULL)
		goto cleanup;
	
	
	if(snprintf(filepath, 256, "%s/%s.dat", temp, filename) >= 256){
		/* snprintf can truncate strings, so malicious user can use it to access another file.
		 * we check string len such way to ensure none of strings was truncated */
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
		
		buffer_write(buffer, 0, &new_key, sizeof(off_t)); 
		
		data->file_stat_status = STAT_NEED_UPDATE;
		
	pthread_mutex_unlock(mutex);
	
	return sizeof(off_t);
}

static ssize_t file_set(chain_t *chain, request_t *request, buffer_t *buffer){
	ssize_t           ret = -1;
	
	off_t             key;
	unsigned int      value_size;
	unsigned int      write_size;
	file_user_data   *data        = ((file_user_data *)chain->user_data);
	
	if(hash_get_copy (request, "key",   TYPE_INT64,  &key,        sizeof(key)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "size",  TYPE_INT32,  &value_size, sizeof(value_size)) != 0)
		return -EINVAL;
	
redo:
	write_size = 0;
	
	buffer_process(buffer, value_size, 0,
		do {
			ret = pwrite(
				((file_user_data *)chain->user_data)->handle,
				chunk,
				size,
				key + offset
			);
			if(ret == -1){
				if(errno == EINTR) goto redo;
				return -errno;
			}
			
			write_size += ret;
			
			if(ret != size)
				goto exit;
		}while(0)
	);
exit:
	data->file_stat_status = STAT_NEED_UPDATE; // coz can write to end, without calling create
	
	return write_size;
}

static ssize_t file_get(chain_t *chain, request_t *request, buffer_t *buffer){
	int               fd;
	ssize_t           ret;
	off_t             key;
	unsigned int      value_size;
	unsigned int      read_size;
	
	if(hash_get_copy (request, "key",   TYPE_INT64, &key,        sizeof(key)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "size",  TYPE_INT32, &value_size, sizeof(value_size)) != 0)
		return -EINVAL;
	
	fd = ((file_user_data *)chain->user_data)->handle;
	
redo:
	read_size = 0;
	
	buffer_process(buffer, value_size, 1,
		do{
			ret = pread(fd, chunk, size, key + offset);
			if(ret == -1){
				if(errno == EINTR) goto redo;
				return -errno;
			}
			
			read_size += ret;
			
			if(ret != size)
				goto exit;
		}while(0)
	);
exit:
	return read_size;
}

static ssize_t file_delete(chain_t *chain, request_t *request, buffer_t *buffer){
	int               fd;
	off_t             key;
	pthread_mutex_t  *mutex;
	unsigned int      value_size;
	unsigned int      forced;
	file_user_data   *data        = ((file_user_data *)chain->user_data);
	
	if(hash_get_copy (request, "key",          TYPE_INT64, &key,        sizeof(key)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "size",         TYPE_INT32, &value_size, sizeof(value_size)) != 0)
		return -EINVAL;
	if(hash_get_copy (request, "imcrazybitch", TYPE_INT32, &forced,     sizeof(forced)) != 0){
		forced = 0;
	}
	
	fd    =  data->handle;
	mutex = &data->create_lock;
	
	pthread_mutex_lock(mutex);
		
		if(file_update_count(data) == STAT_ERROR)
			return -EFAULT;
		
		if(key + value_size != data->file_stat.st_size && forced == 0) // truncating only last elements
			return -EFAULT;
		
		if(ftruncate(fd, key) == -1)
			return -errno;
		
		data->file_stat_status = STAT_NEED_UPDATE;
		
	pthread_mutex_unlock(mutex);
	
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
	if(
		!(
			hash_get_copy (request, "size", TYPE_INT32, &req_size, sizeof(req_size)) == 0  &&
			(move_size = req_size) != (unsigned int)-1
		)
	){
		if(file_update_count(data) == STAT_ERROR)
			return -EINVAL;
		
		move_size = (data->file_stat.st_size - from);
	}
	
	if(from == to || move_size == 0)
		return 0;
	
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
	
	if( (buff = malloc(max_size)) == NULL)
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
		
		if(direction ==  1){ from += size; }else{ from -= size; }
		if(direction ==  1){ to   += size; }else{ to   -= size; }
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
	
	return buffer_write(buffer, 0, &(data->file_stat.st_size), sizeof(data->file_stat.st_size));
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

