#include <libfrozen.h>
#include <fd_t.h>

#include <enum/format/format_t.h>

static ssize_t data_fd_t_read(data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret;
	fd_t                  *fdata             = (fd_t *)data->ptr;
	
redo_read:
	if(fdata->seekable == 0){
		ret = read(
			fdata->fd,
			fargs->buffer,
			fargs->buffer_size
		);
	}else{
		ret = pread(
			fdata->fd,
			fargs->buffer,
			fargs->buffer_size,
			fargs->offset
		);
	}
	if(ret == -1){
		if(errno == EINTR) goto redo_read;
		return -errno;
	}
	if(ret == 0 && fargs->buffer_size != 0){
		fargs->buffer_size = 0;
		return -1; // EOF
	}
	
	fargs->buffer_size = ret;
	return 0;
} // }}}
static ssize_t data_fd_t_write(data_t *data, fastcall_write *fargs){ // {{{
	ssize_t                ret;
	fd_t                  *fdata             = (fd_t *)data->ptr;
	
redo_write:
	if(fdata->seekable == 0){
		ret = write(
			fdata->fd,
			fargs->buffer,
			fargs->buffer_size
		);
	}else{
		ret = pwrite(
			fdata->fd,
			fargs->buffer,
			fargs->buffer_size,
			fargs->offset
		);
	}
	if(ret == -1){
		if(errno == EINTR) goto redo_write;
		return -errno;
	}
	
	fargs->buffer_size = ret;
	return 0;
} // }}}
static ssize_t data_fd_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	char                   buffer[DEF_BUFFER_SIZE];
	fd_t                  *fdata;
	
	if( (fdata = dst->ptr) == NULL){ // no buffer, alloc new
		if( (fdata = dst->ptr = calloc(1, sizeof(fd_t))) == NULL)
			return -ENOMEM;
	}
	
	switch(fargs->format){
		case FORMAT(clean):
		case FORMAT(human):
		case FORMAT(config):;
			fastcall_read r_read = { { 5, ACTION_READ }, 0, buffer, sizeof(buffer) };
			if( (ret = data_query(fargs->src, &r_read)) < 0)
				return ret;
			buffer[r_read.buffer_size] = '\0';
			
			     if(strcasecmp(buffer, "stdin")  == 0){ fdata->fd = 0; fdata->seekable = 0; }
			else if(strcasecmp(buffer, "stdout") == 0){ fdata->fd = 1; fdata->seekable = 0; }
			else if(strcasecmp(buffer, "stderr") == 0){ fdata->fd = 2; fdata->seekable = 0; }
			else
				return -EINVAL;
			
			return 0;
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}

data_proto_t fd_t_proto = {
	.type                   = TYPE_FDT,
	.type_str               = "fd_t",
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_READ]         = (f_data_func)&data_fd_t_read,
		[ACTION_WRITE]        = (f_data_func)&data_fd_t_write,
		[ACTION_CONVERT_FROM] = (f_data_func)&data_fd_t_convert_from,
	}
};
