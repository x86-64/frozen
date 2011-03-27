#include <libfrozen.h>
#include <backend_t.h>
#include <uint/uint32_t.h>
#include <uint/off_t.h>
#include <uint/size_t.h>
#include <raw/raw_t.h>

static ssize_t   data_backend_t_read  (data_t *data, data_ctx_t *ctx, off_t offset, void **buffer, size_t *size){
	ssize_t         ret;
	off_t           d_offset = 0;
	
	hash_data_copy(ret, TYPE_OFFT, d_offset, ctx, HK(offset));
	d_offset += offset;
	
	request_t  req_read[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_READ)  },
		{ HK(offset), DATA_PTR_OFFT(&d_offset)      },
		{ HK(size),   DATA_PTR_SIZET(size)          },
		{ HK(buffer), DATA_RAW(*buffer, *size)      },
		hash_end
	};
	ret = backend_pass( *(backend_t **)(data->data_ptr), req_read);
	if(ret < 0){
		*size = 0;
		return ret;
	}
	
	*size = ret;
	if(ret == 0)
		return -1; // EOF
	
	return ret;
}

static ssize_t   data_backend_t_write (data_t *data, data_ctx_t *ctx, off_t offset, void *buffer, size_t size){
	ssize_t         ret;
	off_t           d_offset = 0;
	
	hash_data_copy(ret, TYPE_OFFT, d_offset, ctx, HK(offset));
	d_offset += offset;
	
	request_t  req_write[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_WRITE) },
		{ HK(offset), DATA_PTR_OFFT(&d_offset)      },
		{ HK(size),   DATA_SIZET(size)              },
		{ HK(buffer), DATA_RAW(buffer, size)        },
		hash_end
	};
	ret = backend_pass( *(backend_t **)(data->data_ptr), req_write);
	return ret;
}

data_proto_t backend_t_proto = {
	.type                   = TYPE_BACKENDT,
	.type_str               = "backend_t",
	.size_type              = SIZE_VARIABLE,
	.func_read              = &data_backend_t_read,
	.func_write             = &data_backend_t_write
};
