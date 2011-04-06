#include <libfrozen.h>

/**
 * @file file.c
 * @ingroup modules
 * @brief File module
 *
 * File module can read and write to file
 */
/**
 * @ingroup modules
 * @addtogroup mod_file Module 'file'
 */
/**
 * @ingroup mod_file
 * @page page_file_config File configuration
 * 
 * Accepted configuration:
 * @code
 * 	{
 * 	        class    = "file",
 * 		filename = "somefilename.dat",        # produce {homedir}{filename} file
 *              filename = {                          # produce concat'ed string from following components:
 *                           homedir => (void_t)'',   #  - {homedir}
 *                           string  => "somename",   #  - any string, any times
 *                           string  => ".dat",       #
 *                           random  => "AAAA",       #  - random string [a-zA-Z0-9] of length 4
 *                           fork    => "keyname"     #  - key from fork request array, strings only
 *                         },
 *              buffer_size = (size_t)'1024',         # default buffer size for move operation
 *              readonly    = (size_t)'1',            # make file read-only, default is read-write
 *              exclusive   = (size_t)'1',            # pass O_EXCL for open, see man page
 *              create      = (size_t)'1',            # pass O_CREAT for open, see man page
 *              mode        = (size_t)'0777',         # file mode on creation. TODO octal representation
 *              retry       = (size_t)'1',            # regen filename and retry open. usefull for example for tmpname generation
 * 	}
 * @endcode
 */

#define EMODULE         1
#define DEF_BUFFER_SIZE 1024

typedef enum file_stat_status {
	STAT_NEED_UPDATE,
	STAT_UPDATED,
	STAT_ERROR
} file_stat_status;

typedef struct file_userdata {
	char *           path;
	int              handle;
	size_t           buffer_size;
	struct stat      file_stat;
	file_stat_status file_stat_status;
	
	pthread_mutex_t  create_lock;
	
	data_t           file_io;
} file_userdata;

// IO's
static ssize_t file_io_write(data_t *data, data_ctx_t *context, off_t offset, void *buffer, size_t size){ // {{{
	ssize_t      ret;
	DT_UINT32T     handle;
	DT_OFFT      key      = 0;
	DT_SIZET     size_min = size;
	
	(void)data;
	
	hash_data_copy(ret, TYPE_UINT32T, handle,    context, HK(handle)); if(ret != 0) return error("handle not supplied");
	hash_data_copy(ret, TYPE_OFFT,  key,       context, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, size_min,  context, HK(size));
	
	// apply size limits
	size_min = MIN(size, size_min);
	
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
	ssize_t    ret;
	DT_UINT32T   handle;
	DT_OFFT    key  = 0;
	DT_SIZET   size = *buffer_size;
	
	(void)data;
	
	hash_data_copy(ret, TYPE_UINT32T, handle,    context, HK(handle)); if(ret != 0) return error("handle not supplied");
	hash_data_copy(ret, TYPE_OFFT,  key,       context, HK(offset));
	hash_data_copy(ret, TYPE_SIZET, size,      context, HK(size));
	
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
static int               file_new_offset(backend_t *backend, off_t *new_key, unsigned int size){ // {{{
	int                    fd;
	int                    ret;
	off_t                  key;
	file_userdata         *data              = ((file_userdata *)backend->userdata);
	
	fd = data->handle;
	
	pthread_mutex_lock(&data->create_lock);
		
		key = lseek(fd, 0, SEEK_END);
		ret = ftruncate(fd, key + size);
		if(ret != -1){
			data->file_stat_status = STAT_NEED_UPDATE;
			*new_key               = key;
			ret                    = 0;
		}else{
			ret                    = -errno;
		}
		
	pthread_mutex_unlock(&data->create_lock);
	
	return ret;
} // }}}
static file_stat_status  file_update_count(file_userdata *data){ // {{{
	int                    ret;
	
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
static char *            file_gen_filepath_from_hasht(config_t *config, config_t *fork_req){ // {{{
	size_t                 ret;
	char                  *str;
	size_t                 str_size;
	char                  *p;
	char                  *buffer            = NULL;
	size_t                 buffer_size       = 0;
	data_t                *curr_data;
	
	do{
		curr_data = hash_item_data(config);
		
		switch( hash_item_key(config) ){
			case HK(fork):;
				hash_key_t      key;
				
				str_size = 0;
				
				if(fork_req == NULL) break;
				
				str = GET_TYPE_STRINGT(curr_data);
				
				if( (key = hash_string_to_key(str)) == 0) break;
				
				hash_data_copy(ret, TYPE_STRINGT, str, fork_req, key);
				if(ret != 0)
					break;
				
				str_size = strlen(str);
				break;
			case HK(string):
				str = GET_TYPE_STRINGT(curr_data);
				str_size = strlen(str);
				break;
			case HK(homedir):
				hash_data_copy(ret, TYPE_STRINGT, str, global_settings, HK(homedir));
				if(ret != 0)
					str = "./";
				str_size = strlen(str);
				break;
			case HK(random):
				if( (str_size = data_value_len(curr_data)) <= 1)
					str_size = 2;
				str_size--;
				break;
			default:
				goto error;
		};
		
		if(str_size > 0){
			if( (buffer = realloc(buffer, buffer_size + str_size + 1)) == NULL)
				break;
			p = buffer + buffer_size;
			
			switch( hash_item_key(config) ){
				case HK(random):;
					intmax_t           i;
					static const char  abc[] = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDGHJKLZXCVBNM1234567890";
					
					for(i = 1; i <= str_size; i++, p++)
						*p = abc[ random() % (sizeof(abc) - 1) ];
					
					break;
				default: memcpy(p, str, str_size); break;
			};
			buffer_size += str_size;
			buffer[buffer_size] = '\0';
		}
	}while( (config = hash_item_next(config)) != NULL);
	
	return buffer;
error:
	if(buffer)
		free(buffer);
	return NULL;
} // }}}
static char *            file_gen_filepath(config_t *config, config_t *fork_req){ // {{{
	char                  *filename_str;
	hash_t                *filename_hash;
	data_t                *filename_data;
	
	if( (filename_hash = hash_find(config, HK(filename))) == NULL)
		return NULL;
	
	filename_data = hash_item_data(filename_hash);
	switch(data_value_type(filename_data)){
		case TYPE_STRINGT:
			filename_str = GET_TYPE_STRINGT(filename_data);
			
			config_t r_gen[] = {
				{ HK(homedir), DATA_STRING("")                    },
				{ HK(string),  DATA_PTR_STRING_AUTO(filename_str) },
				hash_end
			};
			return file_gen_filepath_from_hasht(r_gen, fork_req);
		case TYPE_HASHT:
			filename_hash = GET_TYPE_HASHT(filename_data);
			
			return file_gen_filepath_from_hasht(filename_hash, fork_req);
		default:
			break;
	};
	return NULL;
} // }}}
static int               file_configure_any(backend_t *backend, config_t *config, config_t *fork_req){ // {{{
	ssize_t                ret;
	int                    handle;
	char                  *filepath;
	size_t                 flags             = 0;
	size_t                 cfg_buffsize      = DEF_BUFFER_SIZE;
	size_t                 cfg_rdonly        = 0;
	size_t                 cfg_excl          = 0;
	size_t                 cfg_creat         = 1;
	size_t                 cfg_retry         = 0;
	size_t                 cfg_mode          = S_IRUSR | S_IWUSR;
	file_userdata         *userdata          = (file_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_SIZET,   cfg_buffsize, config, HK(buffer_size));
	hash_data_copy(ret, TYPE_SIZET,   cfg_rdonly,   config, HK(readonly));
	hash_data_copy(ret, TYPE_SIZET,   cfg_excl,     config, HK(exclusive));
	hash_data_copy(ret, TYPE_SIZET,   cfg_creat,    config, HK(create));
	hash_data_copy(ret, TYPE_SIZET,   cfg_mode,     config, HK(mode));
	hash_data_copy(ret, TYPE_SIZET,   cfg_retry,    config, HK(retry));
	
retry:
	if( (filepath = file_gen_filepath(config, fork_req)) == NULL)
		return error("filepath invalid");
	
	flags |= cfg_rdonly == 1 ? O_RDONLY : O_RDWR;
	flags |= cfg_excl   == 1 ? O_EXCL   : 0;
	flags |= cfg_creat  == 1 ? O_CREAT  : 0;
	flags |= O_LARGEFILE;
	
	if( (handle = open(filepath, flags, cfg_mode)) == -1){
		if(cfg_retry == 1) goto retry;
		
		ret = error("file open() error");
		goto cleanup;
	}
	
	userdata->path             = filepath;
	userdata->handle           = handle;
	userdata->buffer_size      = cfg_buffsize;
	userdata->file_stat_status = STAT_NEED_UPDATE;
	
	pthread_mutex_init(&(userdata->create_lock), NULL);
	return 0;
	
cleanup:
	free(filepath);
	return ret;
} // }}}

// Init and destroy
static int file_init(backend_t *backend){ // {{{
	file_userdata *userdata = calloc(1, sizeof(file_userdata));
	if(userdata == NULL)
		return error("calloc returns null");
	
	backend->userdata = userdata;
	
	return 0;
} // }}}
static int file_destroy(backend_t *backend){ // {{{
	file_userdata *data = (file_userdata *)backend->userdata;
	
	if(data->handle){
		free(data->path);
		close(data->handle);
		pthread_mutex_destroy(&(data->create_lock));
	}
	
	free(backend->userdata);
	return 0;
} // }}}
static int file_fork(backend_t *backend, config_t *request){ // {{{
	return file_configure_any(backend, backend->config, request);
} // }}}
static int file_configure(backend_t *backend, config_t *config){ // {{{
	return file_configure_any(backend, config, NULL);
} // }}}

// Requests
static ssize_t file_write(backend_t *backend, request_t *request){ // {{{
	data_t           *buffer;
	data_ctx_t       *buffer_ctx;
	file_userdata    *data        = ((file_userdata *)backend->userdata);
	
	data->file_stat_status = STAT_NEED_UPDATE;
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	data_ctx_t  file_ctx[] = {
		{ HK(handle), DATA_UINT32T(data->handle) },
		hash_next(request)
	};
	
	return data_transfer(&file_io, file_ctx, buffer, buffer_ctx);
} // }}}
static ssize_t file_read(backend_t *backend, request_t *request){ // {{{
	data_t           *buffer;
	data_ctx_t       *buffer_ctx;
	file_userdata    *data        = ((file_userdata *)backend->userdata);
	
	data->file_stat_status = STAT_NEED_UPDATE;
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	data_ctx_t  file_ctx[] = {
		{ HK(handle), DATA_UINT32T(data->handle) },
		hash_next(request)
	};
	
	return data_transfer(buffer, buffer_ctx, &file_io, file_ctx);
} // }}}
static ssize_t file_create(backend_t *backend, request_t *request){ // {{{
	DT_SIZET          size;
	ssize_t           ret;
	off_t             offset;
	data_t            offset_data = DATA_PTR_OFFT(&offset);
	data_t           *key_out;
	data_ctx_t       *key_out_ctx;
	
	hash_data_copy(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return warning("size not supplied");
	
	if( file_new_offset(backend, &offset, size) != 0)
		return error("file expand error");
	
	/* optional offset fill */
	hash_data_find(request, HK(offset_out), &key_out, &key_out_ctx);
	data_transfer(key_out, key_out_ctx, &offset_data, NULL);
	
	/* optional write from buffer */
	request_t r_write[] = {
		{ HK(offset), offset_data },
		hash_next(request)
	};
	if( (ret = file_write(backend, r_write)) != -EINVAL)
		return ret;
	
	return 0;
} // }}}
static ssize_t file_delete(backend_t *backend, request_t *request){ // {{{
	ssize_t           ret;
	int               fd;
	DT_OFFT           key;
	DT_SIZET          size;
	DT_UINT32T          forced = 0;
	file_userdata    *data        = ((file_userdata *)backend->userdata);
	
	hash_data_copy(ret, TYPE_UINT32T, forced, request, HK(forced));
	hash_data_copy(ret, TYPE_OFFT,  key,    request, HK(offset));  if(ret != 0) return warning("offset not supplied");
	hash_data_copy(ret, TYPE_SIZET, size,   request, HK(size));    if(ret != 0) return warning("size not supplied");
	
	fd  = data->handle;
	ret = 0;
	pthread_mutex_lock(&data->create_lock);
		
		if(file_update_count(data) == STAT_ERROR){
			ret = error("file_update_count failed");
			goto exit;
		}
		
		if(
			key + size != data->file_stat.st_size
			&& forced == 0
		){ // truncating only last elements
			ret = warning("cant delete not last elements");
			goto exit;
		}
		
		if(ftruncate(fd, key) == -1){
			ret = -errno;
			goto exit;
		}
		
		data->file_stat_status = STAT_NEED_UPDATE;
		
exit:		
	pthread_mutex_unlock(&data->create_lock);
	return 0;
} // }}}
static ssize_t file_move(backend_t *backend, request_t *request){ // {{{
	size_t           size, max_size;
	ssize_t          ssize, ret;
	char *           buff;
	int              direction;
	file_userdata   *data        = ((file_userdata *)backend->userdata);
	DT_OFFT          from, to;
	DT_SIZET         move_size = -1;
	
	hash_data_copy(ret, TYPE_OFFT,   from,      request, HK(offset_from)); if(ret != 0) return warning("offset_from not supplied");
	hash_data_copy(ret, TYPE_OFFT,   to,        request, HK(offset_to));   if(ret != 0) return warning("offset_to not supplied");
	hash_data_copy(ret, TYPE_SIZET,  move_size, request, HK(size));
	
	if(move_size == -1){
		if(file_update_count(data) == STAT_ERROR)
			return error("file_update_count failed");
		
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
		return warning("size is null");
	
	if( (buff = malloc(max_size)) == NULL)
		return error("malloc error");
	
	ret = 0;
	while(move_size > 0){
		if( (ssize = pread(data->handle, buff, size, from)) == -1){
			if(errno == EINTR) continue;
			ret = -errno;
			break;
		}
		if(ssize == 0){
			ret = error("io error");
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
static ssize_t file_count(backend_t *backend, request_t *request){ // {{{
	data_t          *buffer;
	data_ctx_t      *buffer_ctx;
	file_userdata   *data = ((file_userdata *)backend->userdata);
	
	hash_data_find(request, HK(buffer), &buffer, &buffer_ctx);
	if(buffer == NULL)
		return warning("no buffer supplied");
	
	if(file_update_count(data) == STAT_ERROR)
		return error("file_update_count failed");
	
	data_t count = DATA_PTR_OFFT(&(data->file_stat.st_size));
	
	return data_transfer(
		buffer, buffer_ctx,
		&count,  NULL
	);
} // }}}

backend_t file_proto = {
	.class          = "file",
	.supported_api  = API_CRWD,
	.func_init      = &file_init,
	.func_fork      = &file_fork,
	.func_configure = &file_configure,
	.func_destroy   = &file_destroy,
	{{
		.func_create = &file_create,
		.func_set    = &file_write,
		.func_get    = &file_read,
		.func_delete = &file_delete,
		.func_move   = &file_move,
		.func_count  = &file_count
	}}
};


