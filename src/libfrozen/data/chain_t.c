#include <libfrozen.h>

ssize_t   data_chain_t_read  (data_t *data, data_ctx_t *ctx, off_t offset, void **buffer, size_t *size){
	ssize_t         ret;
	hash_t         *temp;
	off_t           d_offset;
	
	d_offset =
		( (temp = hash_find_typed(ctx, TYPE_OFFT, HK(offset))) != NULL) ?
		HVALUE(temp, off_t) : 0;
	d_offset += offset;
	
	request_t  req_read[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_READ)  },
		{ HK(offset),    DATA_PTR_OFFT(&d_offset)      },
		{ HK(size),   DATA_PTR_SIZET(size)          },
		{ HK(buffer), DATA_MEM(*buffer, *size)      },
		hash_end
	};
	ret = chain_next_query( *(chain_t **)(data->data_ptr), req_read);
	if(ret < 0){
		*size = 0;
		return ret;
	}
	
	*size = ret;
	if(ret == 0)
		return -1; // EOF
	
	return ret;
}

ssize_t   data_chain_t_write (data_t *data, data_ctx_t *ctx, off_t offset, void *buffer, size_t size){
	ssize_t         ret;
	hash_t         *temp;
	off_t           d_offset;
	
	d_offset =
		( (temp = hash_find_typed(ctx, TYPE_OFFT, HK(offset))) != NULL) ?
		HVALUE(temp, off_t) : 0;
	d_offset += offset;
	
	request_t  req_write[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_WRITE) },
		{ HK(offset),    DATA_PTR_OFFT(&d_offset)      },
		{ HK(size),   DATA_SIZET(size)              },
		{ HK(buffer), DATA_MEM(buffer, size)        },
		hash_end
	};
	ret = chain_next_query( *(chain_t **)(data->data_ptr), req_write);
	return ret;
}

/*
REGISTER_TYPE(TYPE_CHAINT)
REGISTER_MACRO(`DATA_CHAINT(_chain)', `TYPE_CHAINT, (chain_t *[]){ _chain }, 0')
REGISTER_PROTO(
	`{
		.type                   = TYPE_CHAINT,
		.type_str               = "chain_t",
		.size_type              = SIZE_VARIABLE,
		.func_read              = &data_chain_t_read,
		.func_write             = &data_chain_t_write
	}'
)
*/

