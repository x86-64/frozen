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

static int file_configure(chain_t *chain, hash_t *config){
	char   *filepath;
	char   *filename;
	char   *temp;
	int     handle;
	size_t  copy_buffer_size;
	
	if(hash_get_typed(config, TYPE_STRING, "filename", (void **)&filename, NULL) != 0)
		return -EINVAL;
	
	filepath  = malloc(256);
	*filepath = '\0';
	
	if(hash_get_typed(global_settings, TYPE_STRING, "homedir", (void **)&temp, NULL) != 0)
		temp = ".";
	
	if(snprintf(filepath, 256, "%s/%s", temp, filename) >= 256){
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
	
	copy_buffer_size =
		(hash_get_typed(config, TYPE_INT32, "copy_buffer_size", (void **)&temp, NULL) == 0) ?
		*(unsigned int *)temp : 512;
	
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

static int file_new_offset(chain_t *chain, off_t *new_key, unsigned int size){
	int               fd;
	int               ret;
	off_t             key;
	pthread_mutex_t  *mutex;
	file_user_data   *data        = ((file_user_data *)chain->user_data);
	
	fd    =  data->handle;
	mutex = &data->create_lock;
	
	pthread_mutex_lock(mutex);
		
		key = lseek(fd, 0, SEEK_END);
		ret = ftruncate(fd, key + size);
		if(ret != -1){
			data->file_stat_status = STAT_NEED_UPDATE;
			*new_key               = key;
			ret                    = 0;
		}else{
			ret                    = -errno;
		}
		
	pthread_mutex_unlock(mutex);
	
	return ret;
}

static ssize_t file_create(chain_t *chain, request_t *request, buffer_t *buffer){
	off_t             new_key;
	hash_t           *r_value_size;
	
	if( (r_value_size = hash_find_typed(request, TYPE_INT32, "size")) == NULL)
		return -EINVAL;
	
	if( file_new_offset(chain, &new_key, HVALUE(r_value_size, unsigned int)) != 0)
		return -EFAULT;
	
	if(buffer != NULL){
		buffer_write(buffer, 0, &new_key, sizeof(off_t)); 
		return sizeof(off_t);
	}
	return 0;
}

static ssize_t file_set(chain_t *chain, request_t *request, buffer_t *buffer){
	unsigned int      write_size;
	off_t             key;
	int               create_mode =  0;
	ssize_t           ret         = -1;
	
	hash_t           *r_key;
	hash_t           *r_value_size;
	file_user_data   *data        = ((file_user_data *)chain->user_data);
	
	if( (r_value_size = hash_find_typed(request, TYPE_INT32, "size")) == NULL)
		return -EINVAL;
	
	if( (r_key        = hash_find_typed(request, TYPE_OFFT,  "key"))  != NULL){
		key = HVALUE(r_key, off_t);
	}else{
		if( file_new_offset(chain, &key, HVALUE(r_value_size, unsigned int)) != 0)
			return -EFAULT;
		
		create_mode = 1;
	}
	
redo:
	write_size = 0;
	
	buffer_process(buffer, HVALUE(r_value_size, unsigned int), 0,
		do {
			ret = pwrite(
				((file_user_data *)chain->user_data)->handle,
				chunk,
				size,
				key + offset
			);
			if(ret == -1){
				if(errno == EINTR) goto redo;
				goto exit;
			}
			
			write_size += ret;
			
			if(ret != size)
				goto exit;
		}while(0)
	);
exit:
	data->file_stat_status = STAT_NEED_UPDATE; // coz can write to end, without calling create
	
	if(buffer != NULL && create_mode == 1){
		buffer_write(buffer, 0, &key, sizeof(key));
	}
	return (create_mode == 1) ? sizeof(off_t) : write_size;
}

static ssize_t file_get(chain_t *chain, request_t *request, buffer_t *buffer){
	int               fd;
	ssize_t           ret;
	unsigned int      read_size;
	hash_t           *r_key;
	hash_t           *r_value_size;
	
	if( (r_key        = hash_find_typed(request, TYPE_OFFT,  "key"))  == NULL)
		return -EINVAL;
	if( (r_value_size = hash_find_typed(request, TYPE_INT32, "size")) == NULL)
		return -EINVAL;
	
	fd = ((file_user_data *)chain->user_data)->handle;
	
redo:
	read_size = 0;
	
	buffer_process(buffer, HVALUE(r_value_size, unsigned int), 1,
		do{
			ret = pread(fd, chunk, size, HVALUE(r_key, off_t) + offset);
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
	int               forced = 0;
	ssize_t           ret;
	pthread_mutex_t  *mutex;
	file_user_data   *data        = ((file_user_data *)chain->user_data);
	hash_t           *r_key;
	hash_t           *r_value_size;
	hash_t           *r_force;
	
	
	if( (r_key        = hash_find_typed(request, TYPE_OFFT,  "key"))  == NULL)
		return -EINVAL;
	if( (r_value_size = hash_find_typed(request, TYPE_INT32, "size")) == NULL)
		return -EINVAL;
	if( (r_force      = hash_find_typed(request, TYPE_INT32, "imcrazybitch")) != NULL)
		forced = HVALUE(r_force, int);
	
	fd    =  data->handle;
	mutex = &data->create_lock;
	
	ret = 0;
	pthread_mutex_lock(mutex);
		
		if(file_update_count(data) == STAT_ERROR){
			ret = -EFAULT;
			goto exit;
		}
		
		if(
			HVALUE(r_key, off_t) + HVALUE(r_value_size, unsigned int) != data->file_stat.st_size
			&& forced == 0
		){ // truncating only last elements
			ret = -EFAULT;
			goto exit;
		}
		
		if(ftruncate(fd, HVALUE(r_key, off_t)) == -1){
			ret = -errno;
			goto exit;
		}
		
		data->file_stat_status = STAT_NEED_UPDATE;
		
exit:		
	pthread_mutex_unlock(mutex);
	return 0;
}

static ssize_t file_move(chain_t *chain, request_t *request, buffer_t *buffer){
	off_t            from, to;
	
	size_t           move_size;
	size_t           ssize, size, max_size;
	char *           buff;
	int              direction;
	int              ret         = 0;
	file_user_data  *data        = ((file_user_data *)chain->user_data);
	hash_t           *r_key_from;
	hash_t           *r_key_to;
	hash_t           *r_value_size;
	
	if( (r_key_from   = hash_find_typed(request, TYPE_OFFT, "key_from")) == NULL)
		return -EINVAL;
	if( (r_key_to     = hash_find_typed(request, TYPE_OFFT, "key_to")) == NULL)
		return -EINVAL;
	
	from = HVALUE(r_key_from, off_t);
	to   = HVALUE(r_key_to,   off_t);
	
	if(
		!(
			(r_value_size = hash_find_typed(request, TYPE_INT32, "size")) != NULL &&
			(move_size = (size_t)HVALUE(r_value_size, unsigned int)) != (unsigned int)-1
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
			ret = -EFAULT;
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

