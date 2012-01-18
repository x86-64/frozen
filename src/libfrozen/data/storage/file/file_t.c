#include <libfrozen.h>

#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <numeric/uint/uint_t.h>

#define EMODULE         1

typedef struct file_t {
	int              handle;
} file_t;

static void    file_destroy(file_t *fdata){ // {{{
	if(fdata->handle > 0){
		close(fdata->handle);
	}
	free(fdata);
} // }}}
static ssize_t file_new(file_t **pfdata, config_t *config){ // {{{
	ssize_t                ret;
	file_t                *fdata;
	char                   filepath[DEF_BUFFER_SIZE];
	uintmax_t              flags             = 0;
	data_t                *cfg_filename;
	uintmax_t              cfg_rdonly        = 0;
	uintmax_t              cfg_excl          = 0;
	uintmax_t              cfg_creat         = 1;
	uintmax_t              cfg_retry         = 0;
	uintmax_t              cfg_mode          = S_IRUSR | S_IWUSR;
	
	if( (fdata = calloc(1, sizeof(file_t))) == NULL)
		return error("calloc returns null");
	
	hash_data_copy(ret, TYPE_UINTT, cfg_rdonly,   config, HK(readonly));
	hash_data_copy(ret, TYPE_UINTT, cfg_excl,     config, HK(exclusive));
	hash_data_copy(ret, TYPE_UINTT, cfg_creat,    config, HK(create));
	hash_data_copy(ret, TYPE_UINTT, cfg_mode,     config, HK(mode));
	hash_data_copy(ret, TYPE_UINTT, cfg_retry,    config, HK(retry));
	
	if( (cfg_filename = hash_data_find(config, HK(filename))) == NULL){
		ret = error("filename not supplied");
		goto error;
	}

retry:;
	// get filepath
	fastcall_read r_read = { { 5, ACTION_READ }, 0, filepath, sizeof(filepath) - 1 };
	if(data_query(cfg_filename, &r_read) < 0){
		ret = error("filename invalid");
		goto error;
	}
	filepath[r_read.buffer_size] = '\0';
	
	// assign flags
	flags |= cfg_rdonly == 1 ? O_RDONLY : O_RDWR;
	flags |= cfg_excl   == 1 ? O_EXCL   : 0;
	flags |= cfg_creat  == 1 ? O_CREAT  : 0;
	#ifdef O_LARGEFILE
	flags |= O_LARGEFILE;
	#endif
	
	if( (fdata->handle = open(filepath, flags, cfg_mode)) == -1){
		if(cfg_retry == 1)
			goto retry;
		
		ret = error("file open() error");
		goto error;
	}
	
	*pfdata = fdata;
	return 0;
	
error:
	file_destroy(fdata);
	return ret;
} // }}}

static ssize_t data_file_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->src == NULL)
		return -EINVAL; 
		
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t            *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return file_new((file_t **)&data->ptr, config);
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_file_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr != NULL)
		file_destroy(data->ptr);
	
	return 0;
} // }}}
static ssize_t data_file_t_read(data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret;
	file_t                *fdata             = (file_t *)data->ptr;
	
	if(!fdata)
		return -EINVAL;
	
redo_read:
	ret = pread(
		fdata->handle,
		fargs->buffer,
		fargs->buffer_size,
		fargs->offset
	);
	if(ret == -1){
		if(errno == EINTR) goto redo_read;
		return -errno;
	}
	if(ret == 0 && fargs->buffer_size != 0)
		return -1; // EOF
	
	fargs->buffer_size = ret;
	return 0;
} // }}}
static ssize_t data_file_t_write(data_t *data, fastcall_write *fargs){ // {{{
	ssize_t                ret;
	file_t                *fdata             = (file_t *)data->ptr;
	
	if(!fdata)
		return -EINVAL;
	
redo_write:
	ret = pwrite(
		fdata->handle,
		fargs->buffer,
		fargs->buffer_size,
		fargs->offset
	);
	if(ret == -1){
		if(errno == EINTR) goto redo_write;
		return -errno;
	}
	
	fargs->buffer_size = ret;
	return 0;
} // }}}
static ssize_t data_file_t_len(data_t *data, fastcall_len *fargs){ // {{{
	ssize_t                ret;
	struct stat            stat;
	file_t                *fdata             = (file_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
	if( (ret = fstat(fdata->handle, &stat) != 0) )
		return ret;
	
	fargs->length = stat.st_size;
	return 0;
} // }}}
static ssize_t data_file_t_resize(data_t *data, fastcall_resize *fargs){ // {{{
	file_t                *fdata             = (file_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
	if(ftruncate(fdata->handle, fargs->length) == -1)
		return -errno;
	
	return 0;
} // }}}

data_proto_t file_t_proto = {
	.type                   = TYPE_FILET,
	.type_str               = "file_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_file_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_file_t_free,
		[ACTION_PHYSICALLEN]  = (f_data_func)&data_file_t_len,
		[ACTION_LOGICALLEN]   = (f_data_func)&data_file_t_len,
		[ACTION_READ]         = (f_data_func)&data_file_t_read,
		[ACTION_WRITE]        = (f_data_func)&data_file_t_write,
		[ACTION_RESIZE]       = (f_data_func)&data_file_t_resize,
	}
};

