#include <libfrozen.h>
#include <alloca.h>

#define DEF_BUFFER_SIZE 1024

typedef enum file_stat_status {
	STAT_NEED_UPDATE,
	STAT_UPDATED,
	STAT_ERROR
} file_stat_status;

typedef struct file_user_data {
	char *           path;
	int              handle;
	size_t           buffer_size;
	struct stat      file_stat;
	file_stat_status file_stat_status;
	
	pthread_mutex_t  create_lock;
	
	data_t           file_io;
} file_user_data;


// IO's
static ssize_t file_io_write(data_t *data, data_ctx_t *context, off_t offset, void *buffer, size_t size){ // {{{
	ssize_t ret;
	int     handle;
	off_t   key;
	size_t  size_min = size;
	hash_t *temp;
	
	handle =
		( (temp = hash_find_typed(context, TYPE_INT32, "handle")) != NULL) ?
		HVALUE(temp, int) : 0;
	key    =
		( (temp = hash_find_typed(context, TYPE_OFFT,  "key")) != NULL) ?
		HVALUE(temp, off_t) : 0;
	
	// apply size limits
	if( (temp = hash_find_typed(context, TYPE_SIZET, "size")) != NULL){
		size_min = MIN(size, HVALUE(temp, size_t));
	}
	
redo:   ret = pwrite(
		handle,
		buffer,
		size_min,
		key + offset
	);
	if(ret == -1){
		if(errno == EINTR) goto redo;
	}
	return ret;
} // }}}
static ssize_t file_io_read (data_t *data, data_ctx_t *context, off_t offset, void **buffer, size_t *buffer_size){ // {{{
	ssize_t ret;
	int     handle;
	off_t   key;
	size_t  size;
	hash_t *temp;
	
	size =
		( (temp = hash_find_typed(context, TYPE_SIZET, "size")) != NULL) ?
		HVALUE(temp, size_t) : *buffer_size;
	handle =
		( (temp = hash_find_typed(context, TYPE_INT32, "handle")) != NULL) ?
		HVALUE(temp, int) : 0;
	key =
		( (temp = hash_find_typed(context, TYPE_OFFT,  "key")) != NULL) ?
		HVALUE(temp, off_t) : 0;
	
	if(offset >= size)
		return -1; // EOF
	
	size = (size > *buffer_size) ? *buffer_size : size;
		
redo:   ret = pread(
		handle,
		*buffer,
		size,
		key + offset
	);
	if(ret == -1){
		if(errno == EINTR) goto redo;
		return -errno;
	}
	if(ret == 0 && size != 0)
		return -1; // EOF
	
	*buffer_size = ret;
	return ret;
} // }}}

data_t file_io = DATA_IOT(
	file_io_read,
	file_io_write
);

// internal functions
static int               file_new_offset(chain_t *chain, off_t *new_key, unsigned int size){ // {{{
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
} // }}}
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

// Init and destory
static int file_init(chain_t *chain){ // {{{
	file_user_data *user_data = calloc(1, sizeof(file_user_data));
	if(user_data == NULL)
		return -ENOMEM;
	
	chain->user_data = user_data;
	
	return 0;
} // }}}
static int file_destroy(chain_t *chain){ // {{{
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
} // }}}
static int file_configure(chain_t *chain, hash_t *config){ // {{{
	char   *filepath;
	char   *filename;
	char   *temp;
	int     handle;
	size_t  buffer_size;
	
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
	
	buffer_size =
		(hash_get_typed(config, TYPE_SIZET, "buffer_size", (void **)&temp, NULL) == 0) ?
		*(unsigned int *)temp : DEF_BUFFER_SIZE;
	
	file_user_data *data = (file_user_data *)chain->user_data;
	
	data->path             = filepath;
	data->handle           = handle;
	data->buffer_size      = buffer_size;
	data->file_stat_status = STAT_NEED_UPDATE;
	
	pthread_mutex_init(&(data->create_lock), NULL);
	
	return 0;

cleanup:
	free(filepath);
	return -EINVAL;
} // }}}

// Requests
static ssize_t file_create(chain_t *chain, request_t *request){ // {{{
	off_t             new_key;
	data_t            new_key_data = DATA_PTR_OFFT(&new_key);
	data_t           *key_out;
	data_ctx_t       *key_out_ctx;
	hash_t           *r_value_size;
	//hash_t           *r_buffer;
	
	if( (r_value_size = hash_find_typed(request, TYPE_SIZET, "size")) == NULL)
		return -EINVAL;
	
	if( file_new_offset(chain, &new_key, HVALUE(r_value_size, unsigned int)) != 0)
		return -EFAULT;
	
	/*
	if( (r_buffer = hash_find_typed(request, TYPE_BUFFERT, "buffer")) != NULL ){
		buffer_write(hash_get_value_ptr(r_buffer), 0, &new_key, sizeof(off_t)); 
		return sizeof(off_t);
	}*/
	if( (key_out = hash_get_data(request, "key_out")) != NULL){
		key_out_ctx = hash_get_data_ctx(request, "key_out");
		
		data_transfer(
			key_out,       key_out_ctx,
			&new_key_data, NULL
		);
		return sizeof(off_t);
	}
	return 0;
} // }}}

static ssize_t file_set(chain_t *chain, request_t *request){ // {{{
	data_t           *buffer;
	data_ctx_t       *buffer_ctx;
	
	off_t             def_key;
	data_t            def_key_data = DATA_PTR_OFFT(&def_key);
	data_t           *key, *size;
	
	data_t           *key_out;
	data_ctx_t       *key_out_ctx;
	
	ssize_t           ret         = -1;
	file_user_data   *data        = ((file_user_data *)chain->user_data);
	
	/* get buffer info */
	if( (buffer = hash_get_data(request, "buffer")) == NULL)
		return -EINVAL;
	
	buffer_ctx = hash_get_data_ctx(request, "buffer");
	
	/* get file io related info */
	if( (key = hash_get_data(request, "key")) == NULL){
		size_t  new_size;
		
		new_size = data_len(buffer, buffer_ctx);
		
		if( file_new_offset(chain, &def_key, new_size) != 0)
			return -EFAULT;
		
		key = &def_key_data;
	}
	
	/* prepare contexts */
	hash_t file_ctx_size = hash_null;
	
	// if size supplied - apply it as file output restruction
	if( (size = hash_get_data(request, "size")) != NULL){
		hash_t new_size = { "size", *size };
		
		memcpy(&file_ctx_size, &new_size, sizeof(new_size));
	}
	
	data_ctx_t  file_ctx[] = {
		{ "handle", DATA_INT32(data->handle) },
		{ "key",    *key                     },
		file_ctx_size,
		hash_end
	};
	
	/* write info to file */
	ret = data_transfer(
		&file_io, file_ctx,
		buffer,   buffer_ctx
	);
	
	/* write key if requested */
	if( (key_out = hash_get_data(request, "key_out")) != NULL){
		key_out_ctx = hash_get_data_ctx(request, "key_out");
		
		data_transfer(
			key_out, key_out_ctx,
			key,     NULL
		);
	}
	
	data->file_stat_status = STAT_NEED_UPDATE;
	
	return ret;
} // }}}
static ssize_t file_get(chain_t *chain, request_t *request){ // {{{
	data_t           *buffer;
	data_ctx_t       *buffer_ctx;
	
	data_t           *key, *size;
	
	ssize_t           ret         = -1;
	file_user_data   *data        = ((file_user_data *)chain->user_data);
	
	/* get buffer info */
	if( (buffer = hash_get_data(request, "buffer")) == NULL)
		return -EINVAL;
	buffer_ctx = hash_get_data_ctx(request, "buffer");
	
	/* get file io related info */
	if( (key = hash_get_data(request, "key")) == NULL)
		return -EINVAL;
	
	/* get size restructions */
	hash_t file_ctx_size = hash_null;
	if( (size = hash_get_data(request, "size")) != NULL){
		hash_t new_size = { "size", *size };
		
		memcpy(&file_ctx_size, &new_size, sizeof(new_size));
	}
	
	/* prepare file context */
	data_ctx_t  file_ctx[] = {
		{ "handle", DATA_INT32(data->handle) },
		{ "key",    *key                     },
		file_ctx_size,
		hash_end
	};
	
	/* read info from file */
	ret = data_transfer(
		buffer,   buffer_ctx,
		&file_io, file_ctx
	);
	
	return ret;
} // }}}
static ssize_t file_delete(chain_t *chain, request_t *request){ // {{{
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
	if( (r_value_size = hash_find_typed(request, TYPE_SIZET, "size")) == NULL)
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
} // }}}
static ssize_t file_move(chain_t *chain, request_t *request){ // {{{
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
			(r_value_size = hash_find_typed(request, TYPE_SIZET, "size")) != NULL &&
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
		max_size   = ((to - from) > data->buffer_size) ? data->buffer_size : (size_t)(to - from);
		size       = (max_size < move_size)            ? max_size          : move_size;
		
		from      += move_size - size;
		to        += move_size - size;
		direction  = -1;
	}else{
		max_size   = ((from - to) > data->buffer_size) ? data->buffer_size : (size_t)(from - to);
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
} // }}}
static ssize_t file_count(chain_t *chain, request_t *request){ // {{{
	buffer_t        *buffer;
	file_user_data  *data = ((file_user_data *)chain->user_data);
	
	if( hash_get_typed(request, TYPE_BUFFERT, "buffer", (void **)&buffer, NULL) != 0)
		return -EINVAL;
	
	if(file_update_count(data) == STAT_ERROR)
		return -EINVAL;
	
	return buffer_write(buffer, 0, &(data->file_stat.st_size), sizeof(data->file_stat.st_size));
} // }}}

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

