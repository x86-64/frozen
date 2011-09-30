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
 * @page page_file_config Configuration
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
 *                           fork    => "keyname"     #  - key from fork request array, strings only. Skipped on non-fork requests
 *                         },
 *              buffer_size = (size_t)'1024',         # default buffer size for move operation
 *              readonly    = (size_t)'1',            # make file read-only, default is read-write
 *              exclusive   = (size_t)'1',            # pass O_EXCL for open, see man page
 *              create      = (size_t)'1',            # pass O_CREAT for open, see man page
 *              mode        = (size_t)'0777',         # file mode on creation. TODO octal representation
 *              retry       = (size_t)'1',            # regen filename and retry open. usefull for example for tmpname generation
 *              fork_only   = (size_t)'1',            # open new file only on forks
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
	char *           path;            // string with full file path, or NULL if fork_only selected
	int              handle;          // real file handle, or -1 if file suspended
	size_t           flags;
	size_t           buffer_size;
	struct stat      file_stat;
	file_stat_status file_stat_status;
	
	pthread_mutex_t  create_lock;
	
	data_t           file_io;
} file_userdata;

// IO's
static ssize_t file_io_handler(file_userdata *userdata, fastcall_header *hargs){ // {{{
	ssize_t      ret;
	
	switch(hargs->action){
		case ACTION_READ:;
			fastcall_read *rargs = (fastcall_read *)hargs;
			
		redo_read:
			ret = pread(
				userdata->handle,
				rargs->buffer,
				rargs->buffer_size,
				rargs->offset
			);
			if(ret == -1){
				if(errno == EINTR) goto redo_read;
				return -errno;
			}
			if(ret == 0 && rargs->buffer_size != 0)
				return -1; // EOF
			
			rargs->buffer_size = ret;
			return 0;

		case ACTION_WRITE:;
			fastcall_write *wargs = (fastcall_write *)hargs;
		
		redo_write:
			ret = pwrite(
				userdata->handle,
				wargs->buffer,
				wargs->buffer_size,
				wargs->offset
			);
			if(ret == -1){
				if(errno == EINTR) goto redo_write;
				return -errno;
			}
			
			wargs->buffer_size = ret;
			return 0;

		case ACTION_TRANSFER:
			return data_protos[ TYPE_DEFAULTT ]->handlers[ hargs->action ](&userdata->file_io, hargs);

		default:
			break;
	};
	return -ENOSYS;
} // }}}

// internal functions
static int               file_new_offset(backend_t *backend, off_t *new_key, unsigned int size){ // {{{
	int                    fd;
	int                    ret;
	off_t                  key;
	file_userdata         *data              = ((file_userdata *)backend->userdata);
	
	fd = data->handle;
	
	pthread_mutex_lock(&data->create_lock);
		
		if( (key = lseek(fd, 0, SEEK_END)) == -1)
			goto error;
		
		if( (ret = ftruncate(fd, key + size)) == -1)
			goto error;
		
		data->file_stat_status = STAT_NEED_UPDATE;
		*new_key               = key;
		ret                    = 0;
exit:
	pthread_mutex_unlock(&data->create_lock);
	
	return ret;
error:
	ret  = -errno;
	printf("file_new_offset on '%s': %s\n", data->path, strerror(errno));
	goto exit;
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
static ssize_t           file_prepare(file_userdata *userdata){ // {{{
	ssize_t                ret               = 0;
	size_t                 new_flags;
	
	if(userdata->path == NULL)
		return error("called fork_only file");
	
	pthread_mutex_lock(&userdata->create_lock);
		
		if(userdata->handle == -1){
			// resume file
			new_flags = userdata->flags & ~O_CREAT;
			
			if( (userdata->handle = open(userdata->path, new_flags)) == -1 )
				ret = error("resume error");
		}
	
	pthread_mutex_unlock(&userdata->create_lock);

	return ret;
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
				
				// BAD
				str = (char *)(curr_data->ptr);
				
				if( (key = hash_string_to_key(str)) == 0) break;
				
				hash_data_copy(ret, TYPE_STRINGT, str, fork_req, key);
				if(ret != 0)
					break;
				
				str_size = strlen(str);
				break;
			case HK(string):
				str      = (char *)curr_data->ptr; // TODO rewrite to data_read  BAD
				str_size = strlen(str); //GET_TYPE_STRINGT_LEN(curr_data);
				break;
			case HK(homedir):
				hash_data_copy(ret, TYPE_STRINGT, str, global_settings, HK(homedir));
				if(ret != 0)
					str = "./";
				str_size = strlen(str);
				break;
			case HK(random):
				//if( (str_size = data_value_len(curr_data)) <= 1)
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
	hash_t                *filename_hash;
	data_t                *filename_data;
	
	if( (filename_hash = hash_find(config, HK(filename))) == NULL)
		return NULL;
	
	filename_data = hash_item_data(filename_hash);
	switch(filename_data->type){
		case TYPE_STRINGT:;
			config_t r_gen[] = {
				{ HK(homedir), DATA_STRING("")                    },
				{ HK(string),  *filename_data }, // DATA_PTR_STRING_AUTO(filename_str) },
				hash_end
			};
			return file_gen_filepath_from_hasht(r_gen, fork_req);
		case TYPE_HASHT:
			filename_hash = (hash_t *)filename_data->ptr;
			
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
	size_t                 cfg_forkonl       = 0;
	size_t                 cfg_mode          = S_IRUSR | S_IWUSR;
	file_userdata         *userdata          = (file_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_SIZET,   cfg_buffsize, config, HK(buffer_size));
	hash_data_copy(ret, TYPE_SIZET,   cfg_rdonly,   config, HK(readonly));
	hash_data_copy(ret, TYPE_SIZET,   cfg_excl,     config, HK(exclusive));
	hash_data_copy(ret, TYPE_SIZET,   cfg_creat,    config, HK(create));
	hash_data_copy(ret, TYPE_SIZET,   cfg_mode,     config, HK(mode));
	hash_data_copy(ret, TYPE_SIZET,   cfg_retry,    config, HK(retry));
	hash_data_copy(ret, TYPE_SIZET,   cfg_forkonl,  config, HK(fork_only));
	
	if(cfg_forkonl == 1 && fork_req == NULL)
		return 0;

retry:
	if( (filepath = file_gen_filepath(config, fork_req)) == NULL)
		return error("filepath invalid");
	
	flags |= cfg_rdonly == 1 ? O_RDONLY : O_RDWR;
	flags |= cfg_excl   == 1 ? O_EXCL   : 0;
	flags |= cfg_creat  == 1 ? O_CREAT  : 0;
	flags |= O_LARGEFILE;
	
	if( (handle = open(filepath, flags, cfg_mode)) == -1){
		if(cfg_retry == 1){
			free(filepath);
			goto retry;
		}
		
		printf("open error: %s\n", filepath);
		ret = error("file open() error");
		goto cleanup;
	}
	
	data_t file_io = DATA_IOT(userdata, (f_io_func)&file_io_handler);
	
	userdata->path             = filepath;
	userdata->flags            = flags;
	userdata->handle           = handle;
	userdata->buffer_size      = cfg_buffsize;
	userdata->file_stat_status = STAT_NEED_UPDATE;
	
	fastcall_copy r_copy = { { 3, ACTION_COPY }, &userdata->file_io };
	data_query(&file_io, &r_copy); // TODO err handle
	
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
		fastcall_free r_free = { { 2, ACTION_FREE } };
		data_query(&data->file_io, &r_free);
		
		free(data->path);
		close(data->handle);
		pthread_mutex_destroy(&(data->create_lock));
	}
	
	free(backend->userdata);
	return 0;
} // }}}
static int file_fork(backend_t *backend, backend_t *parent, config_t *request){ // {{{
	return file_configure_any(backend, backend->config, request);
} // }}}
static int file_configure(backend_t *backend, config_t *config){ // {{{
	return file_configure_any(backend, config, NULL);
} // }}}


// Requests
static ssize_t file_write(backend_t *backend, request_t *request){ // {{{
	ssize_t           ret;
	uintmax_t         offset                 = 0;
	uintmax_t         size                   = ~0;
	data_t           *buffer;
	file_userdata    *userdata               = ((file_userdata *)backend->userdata);
	
	if( (ret = file_prepare(userdata)) < 0)
		return ret;
	
	userdata->file_stat_status = STAT_NEED_UPDATE;
	
	hash_data_copy(ret, TYPE_UINTT, offset, request, HK(offset));
	hash_data_copy(ret, TYPE_UINTT, size,   request, HK(size));
	
	if( (buffer = hash_data_find(request, HK(buffer))) == NULL)
		return -EINVAL;
	
	data_t d_slice = DATA_SLICET(&userdata->file_io, offset, size);
	
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_slice };
	return data_query(buffer, &r_transfer);
} // }}}
static ssize_t file_read(backend_t *backend, request_t *request){ // {{{
	ssize_t           ret;
	uintmax_t         offset                 = 0;
	uintmax_t         size                   = ~0;
	data_t           *buffer;
	file_userdata    *userdata               = ((file_userdata *)backend->userdata);
	
	if( (ret = file_prepare(userdata)) < 0)
		return ret;
	
	//userdata->file_stat_status = STAT_NEED_UPDATE;
	
	hash_data_copy(ret, TYPE_UINTT, offset, request, HK(offset));
	hash_data_copy(ret, TYPE_UINTT, size,   request, HK(size));
	
	if( (buffer = hash_data_find(request, HK(buffer))) == NULL)
		return -EINVAL;
	
	data_t d_slice = DATA_SLICET(&userdata->file_io, offset, size);
	
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, buffer };
	return data_query(&d_slice, &r_transfer);
} // }}}
static ssize_t file_create(backend_t *backend, request_t *request){ // {{{
	size_t            size;
	ssize_t           ret;
	off_t             offset;
	data_t            offset_data            = DATA_PTR_OFFT(&offset);
	data_t           *key_out;
	file_userdata    *userdata               = ((file_userdata *)backend->userdata);
	
	if( (ret = file_prepare(userdata)) < 0)
		return ret;
	
	hash_data_copy(ret, TYPE_SIZET, size, request, HK(size)); if(ret != 0) return warning("size not supplied");
	
	if( file_new_offset(backend, &offset, size) != 0)
		return error("file expand error");
	
	// optional offset fill
	key_out = hash_data_find(request, HK(offset_out));
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, key_out };
	data_query(&offset_data, &r_transfer);
	
	// optional write from buffer
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
	off_t             key;
	size_t            size;
	uintmax_t         forced                 = 0;
	file_userdata    *userdata               = ((file_userdata *)backend->userdata);
	
	if( (ret = file_prepare(userdata)) < 0)
		return ret;
	
	hash_data_copy(ret, TYPE_UINTT, forced, request, HK(forced));
	hash_data_copy(ret, TYPE_OFFT,  key,    request, HK(offset));  if(ret != 0) return warning("offset not supplied");
	hash_data_copy(ret, TYPE_SIZET, size,   request, HK(size));    if(ret != 0) return warning("size not supplied");
	
	fd  = userdata->handle;
	ret = 0;
	pthread_mutex_lock(&userdata->create_lock);
		
		if(file_update_count(userdata) == STAT_ERROR){
			ret = error("file_update_count failed");
			goto exit;
		}
		
		if(
			key + size != userdata->file_stat.st_size
			&& forced == 0
		){ // truncating only last elements
			ret = warning("cant delete not last elements");
			goto exit;
		}
		
		if(ftruncate(fd, key) == -1){
			ret = -errno;
			goto exit;
		}
		
		userdata->file_stat_status = STAT_NEED_UPDATE;
		
exit:		
	pthread_mutex_unlock(&userdata->create_lock);
	return 0;
} // }}}
static ssize_t file_move(backend_t *backend, request_t *request){ // {{{
	size_t           size, max_size;
	ssize_t          ssize, ret;
	char *           buff;
	int              direction;
	off_t            from, to;
	size_t           move_size               = -1;
	file_userdata   *data                    = ((file_userdata *)backend->userdata);
	
	if( (ret = file_prepare(data)) < 0)
		return ret;
	
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
	ssize_t                ret;
	data_t                *buffer;
	file_userdata         *data              = ((file_userdata *)backend->userdata);
	
	if( (ret = file_prepare(data)) < 0)
		return ret;
	
	if( (buffer = hash_data_find(request, HK(buffer))) == NULL)
		return warning("no buffer supplied");
	
	if(file_update_count(data) == STAT_ERROR)
		return error("file_update_count failed");
	
	data_t count = DATA_PTR_OFFT(&(data->file_stat.st_size));
	
	fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, buffer };
	return data_query(&count, &r_transfer);
} // }}}
static ssize_t file_custom(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	char                  *function;
	file_userdata         *userdata          = ((file_userdata *)backend->userdata);
	
	hash_data_copy(ret, TYPE_STRINGT,  function,  request, HK(function));
	if(ret != 0)
		goto pass;
	
	if(strcmp(function, "file_suspend") == 0){
		pthread_mutex_lock(&userdata->create_lock);
			
			if(userdata->handle != -1){	
				close(userdata->handle);
				userdata->handle = -1;
			}
			
		pthread_mutex_unlock(&userdata->create_lock);
		return 0;
	}
	if(strcmp(function, "file_running") == 0){
		data_t                *buffer;
		uintmax_t              answer            = (userdata->handle == -1) ? 0 : 1;
		data_t                 answer_data       = DATA_PTR_UINTT(&answer);
		
		if( (buffer = hash_data_find(request, HK(buffer))) == NULL)
			return warning("no buffer supplied");
		
		fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, buffer };
		return data_query(&answer_data, &r_transfer);
	}
pass:
	return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
} // }}}

backend_t file_proto = {
	.class          = "storage/file",
	.supported_api  = API_CRWD,
	.func_init      = &file_init,
	.func_fork      = &file_fork,
	.func_configure = &file_configure,
	.func_destroy   = &file_destroy,
	{
		.func_create = &file_create,
		.func_set    = &file_write,
		.func_get    = &file_read,
		.func_delete = &file_delete,
		.func_move   = &file_move,
		.func_count  = &file_count,
		.func_custom = &file_custom
	}
};


