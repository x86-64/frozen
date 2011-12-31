#include <libfrozen.h>
#include <fd_t.h>

static ssize_t data_fd_t_handler (data_t *data, fastcall_header *hargs){ // {{{
	ssize_t                ret;
	fd_t                  *fdata             = (fd_t *)data->ptr;
	
	switch(hargs->action){
		case ACTION_READ:;
			fastcall_read *rargs = (fastcall_read *)hargs;
			
		redo_read:
			if(fdata->seekable == 0){
				ret = read(
					fdata->fd,
					rargs->buffer,
					rargs->buffer_size
				);
			}else{
				ret = pread(
					fdata->fd,
					rargs->buffer,
					rargs->buffer_size,
					rargs->offset
				);
			}
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
			if(fdata->seekable == 0){
				ret = write(
					fdata->fd,
					wargs->buffer,
					wargs->buffer_size
				);
			}else{
				ret = pwrite(
					fdata->fd,
					wargs->buffer,
					wargs->buffer_size,
					wargs->offset
				);
			}
			if(ret == -1){
				if(errno == EINTR) goto redo_write;
				return -errno;
			}
			
			wargs->buffer_size = ret;
			return 0;

		case ACTION_TRANSFER:
		case ACTION_CONVERT_TO:
			return data_protos[ TYPE_DEFAULTT ]->handlers[ hargs->action ](data, hargs);
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}

data_proto_t fd_t_proto = {
	.type                   = TYPE_FDT,
	.type_str               = "fd_t",
	.api_type               = API_DEFAULT_HANDLER,
	.handler_default        = (f_data_func)&data_fd_t_handler
};
