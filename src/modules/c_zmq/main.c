#include <libfrozen.h>
#include <zmq.h>
#include <zeromq_t.h>

#include <errors_list.c>

#define ZMQ_NTHREADS        1

typedef struct zeromq_t {
	void                  *zmq_socket;
	hash_t                *config;
	uintmax_t              lazy;
} zeromq_t;

typedef struct zeromq_t_opt_t {
	int                    option;
	hashkey_t              key;
	uintmax_t              multi_value;
} zeromq_t_opt_t;

zeromq_t_opt_t                 zmq_opts_binary[] = {
	{ ZMQ_IDENTITY,          HK(identity),          0 },
	{ ZMQ_SUBSCRIBE,         HK(subscribe),         1 },
	{ ZMQ_UNSUBSCRIBE,       HK(unsubscribe),       1 },
	{ 0, 0, 0 }
};
zeromq_t_opt_t                 zmq_opts_uint64[] = {
	{ ZMQ_HWM,               HK(hwm),               0 },
	{ ZMQ_AFFINITY,          HK(affinity),          0 },
	{ ZMQ_SNDBUF,            HK(sndbuf_size),       0 },
	{ ZMQ_RCVBUF,            HK(rcvbuf_size),       0 },
	{ 0, 0, 0 }
};
zeromq_t_opt_t                 zmq_opts_int64[] = {
	{ ZMQ_SWAP,              HK(swap),              0 },
	{ ZMQ_RATE,              HK(rate),              0 },
	{ ZMQ_RECOVERY_IVL,      HK(recovery_ivl),      0 },
	{ ZMQ_RECOVERY_IVL_MSEC, HK(recovery_ivl_msec), 0 },
	{ ZMQ_MCAST_LOOP,        HK(mcast_loop),        0 },
	{ 0, 0, 0 }
};
zeromq_t_opt_t                 zmq_opts_int[] = {
	{ ZMQ_LINGER,            HK(linger),            0 },
	{ ZMQ_RECONNECT_IVL,     HK(reconnect_ivl),     0 },
	{ ZMQ_RECONNECT_IVL_MAX, HK(reconnect_ivl_max), 0 },
	{ ZMQ_BACKLOG,           HK(backlog),           0 },
	{ 0, 0, 0 }
};

void                          *zmq_ctx           = NULL;
uintmax_t                      zmq_data          = 0;
pthread_mutex_t                zmq_data_mtx      = PTHREAD_MUTEX_INITIALIZER;

static ssize_t zeromq_t_global_increment(void){ // {{{
	ssize_t                ret               = 0;
	
	pthread_mutex_lock(&zmq_data_mtx);
		if(!zmq_ctx){
			if( (zmq_ctx = zmq_init(ZMQ_NTHREADS)) == NULL){
				pthread_mutex_unlock(&zmq_data_mtx);
				ret = error("zmq_init failed");
				goto exit;
			}
		}
		zmq_data++;
exit:
	pthread_mutex_unlock(&zmq_data_mtx);
	return ret;
} // }}}
static void    zeromq_t_global_decrement(void){ // {{{
	pthread_mutex_lock(&zmq_data_mtx);
		if(zmq_data-- == 0){
			zmq_term(zmq_ctx);
			zmq_ctx = NULL;
		}
	pthread_mutex_unlock(&zmq_data_mtx);
} // }}}

static ssize_t zeromq_t_string_to_type(char *type_str){ // {{{
	if(type_str != NULL){
		if(strcasecmp(type_str, "req") == 0) return ZMQ_REQ;
		if(strcasecmp(type_str, "rep") == 0) return ZMQ_REP;
		if(strcasecmp(type_str, "dealer") == 0) return ZMQ_DEALER;
		if(strcasecmp(type_str, "router") == 0) return ZMQ_ROUTER;
		if(strcasecmp(type_str, "pub") == 0) return ZMQ_PUB;
		if(strcasecmp(type_str, "sub") == 0) return ZMQ_SUB;
		if(strcasecmp(type_str, "push") == 0) return ZMQ_PUSH;
		if(strcasecmp(type_str, "pull") == 0) return ZMQ_PULL;
		if(strcasecmp(type_str, "pair") == 0) return ZMQ_PAIR;
	}
	return -1;
} // }}}

#define get_opt(_key,_func){                                                                                          \
	if( (opt = hash_data_find(fdata->config, _key)) != NULL){                                                     \
		if(data_get_continious(opt, &freeme, &opt_binary_ptr, &opt_binary_size) >= 0){                        \
			if( (p = strndup(opt_binary_ptr, opt_binary_size)) != NULL){                                  \
				_func;                                                                                \
				free(p);                                                                              \
			}else{                                                                                        \
				ret = error("strndup failed");                                                        \
			}                                                                                             \
		}else{                                                                                                \
			ret = error("data_get_continious failed on zmq options");                                     \
		}                                                                                                     \
		data_free(&freeme);                                                                                   \
	}                                                                                                             \
}

static ssize_t zeromq_t_socket_from_config(zeromq_t *fdata){ // {{{
	ssize_t                ret, ret2;
	uintmax_t              ready             = 0;
	char                  *p;
	zeromq_t_opt_t        *c;
	data_t                *opt, freeme;
	intmax_t               opt_int;
	int64_t                opt_int64;
	uint64_t               opt_uint64;
	void                  *opt_binary_ptr;
	uintmax_t              opt_binary_size;
	
	get_opt(HK(type),
		do{
			if( (fdata->zmq_socket = zmq_socket(zmq_ctx, zeromq_t_string_to_type(p))) != NULL){
				ready = 1;
			}else{
				ret = error("zmq_socket failed");
			}
		}while(0)
	);
	if(ready != 1)
		goto error;
	
	// binary options
	for(ret = 0, c = zmq_opts_binary; ret == 0 && c->key != 0; c++){
		if( (opt = hash_data_find(fdata->config, c->key))){
			if(data_get_continious(opt, &freeme, &opt_binary_ptr, &opt_binary_size) >= 0){
				if(zmq_setsockopt(fdata->zmq_socket, c->option, opt_binary_ptr, opt_binary_size) != 0){
					ret = error("zmq_setsockopt binary failed");
				}
			}else{
				ret = error("data_get_continious failed on zmq options");
			}
			data_free(&freeme);
		}
	}
	if(ret != 0)
		goto error;
	
	// uint64 options
	for(ret = 0, c = zmq_opts_uint64; ret == 0 && c->key != 0; c++){
		hash_data_get(ret2, TYPE_UINT64T, opt_uint64, fdata->config, c->key);
		if(ret2 == 0){
			if(zmq_setsockopt(fdata->zmq_socket, c->option, &opt_uint64, sizeof(opt_uint64)) != 0){
				ret = error("zmq_setsockopt uint64 failed");
			}
		}
	}
	if(ret != 0)
		goto error;
	
	// int64 options
	for(ret = 0, c = zmq_opts_int64; ret == 0 && c->key != 0; c++){
		hash_data_get(ret2, TYPE_INT64T, opt_int64, fdata->config, c->key);
		if(ret2 == 0){
			if(zmq_setsockopt(fdata->zmq_socket, c->option, &opt_int64, sizeof(opt_int64)) != 0){
				ret = error("zmq_setsockopt int64 failed");
			}
		}
	}
	if(ret != 0)
		goto error;
	
	// int options
	for(ret = 0, c = zmq_opts_int; ret == 0 && c->key != 0; c++){
		hash_data_get(ret2, TYPE_INTT, opt_int, fdata->config, c->key);
		if(ret2 == 0){
			if(zmq_setsockopt(fdata->zmq_socket, c->option, &opt_int, MIN(sizeof(opt_int), sizeof(int))) != 0){
				ret = error("zmq_setsockopt int failed");
			}
		}
	}
	if(ret != 0)
		goto error;

	get_opt(HK(bind),
		do{
			if(zmq_bind(fdata->zmq_socket, opt_binary_ptr) == 0){
				ready = 2;
			}else{
				ret = error("zmq_bind failed");
			}
		}while(0)
	);
	get_opt(HK(connect),
		do{
			if(zmq_connect(fdata->zmq_socket, opt_binary_ptr) == 0){
				ready = 2;
			}else{
				ret = error("zmq_connect failed");
			}
		}while(0)
	);
	
	if(ready != 2)
		ret = error("no bind nor connect supplied in zmq options");
	
error:
	return ret;
} // }}}

static void    zeromq_t_destroy(zeromq_t *fdata){ // {{{
	if(fdata->lazy == 0){
		if(fdata->zmq_socket)
			zmq_close(fdata->zmq_socket);		
		
		zeromq_t_global_decrement();
	}
	
	if(fdata->config)
		hash_free(fdata->config);
	
	free(fdata);
} // }}}
static ssize_t zeromq_t_prepare(zeromq_t *fdata){ // {{{
	ssize_t                ret;
	
	if(fdata->zmq_socket == NULL && fdata->lazy == 0){ // if no socket yet created and in lazy mode
		if( (ret = zeromq_t_global_increment()) < 0)
			return ret;
		
		if( (ret = zeromq_t_socket_from_config(fdata)) < 0){
			zeromq_t_destroy(fdata);
			return ret;
		}
	}
	return 0;
} // }}}
static ssize_t zeromq_t_new(zeromq_t **pfdata, config_t *config){ // {{{
	ssize_t                ret;
	zeromq_t              *fdata;
	
	if((fdata = calloc(1, sizeof(zeromq_t))) == NULL)
		return -ENOMEM;
	
	fdata->config = config;
	
	hash_data_get(ret, TYPE_UINTT, fdata->lazy, fdata->config, HK(lazy));
	if(fdata->lazy == 0){
		if( (ret = zeromq_t_prepare(fdata)) < 0)
			return ret;
	}
	
	*pfdata = fdata;
	return 0;
} // }}}

static void    zeromq_t_msg_free(void *data, void *hint){ // {{{
	data_free(hint);
	free(hint);
} // }}}

static ssize_t data_zeromq_t_convert_to(data_t *data, fastcall_convert_to *fargs){ // {{{
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	switch(fargs->format){
		case FORMAT(binary):;
			data_t             packed           = DATA_PTR_HASHT(&fdata->config);
			return data_query(&packed, fargs);
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_zeromq_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t            *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0 || config == NULL)
				return -EINVAL;
			
			if( (config = hash_copy(config)) == NULL)
				return -ENOMEM;
			
			return zeromq_t_new((zeromq_t **)&data->ptr, config);
			
		case FORMAT(binary):;
			data_t             packed           = DATA_PTR_HASHT(NULL); // don't free result, used internally 
			
			if( (ret = data_query(&packed, fargs) < 0))
				return ret;
			
			return zeromq_t_new((zeromq_t **)&data->ptr, (hash_t *)packed.ptr);

		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_zeromq_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr != NULL)
		zeromq_t_destroy(data->ptr);
	
	return 0;
} // }}}
static ssize_t data_zeromq_t_read(data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret;
	void                  *msg_data;
	size_t                 msg_data_size;
	zmq_msg_t              zmq_msg;
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	if( (ret = zeromq_t_prepare(fdata)) < 0)
		return ret;
	
	if( zmq_msg_init(&zmq_msg) != 0)
		return error("zmq_msg_init failed");
	
	if( zmq_recv(fdata->zmq_socket, &zmq_msg, 0) != 0)
		return -errno;
	
	msg_data      = zmq_msg_data(&zmq_msg);
	msg_data_size = zmq_msg_size(&zmq_msg);
	
	if(fargs->buffer == NULL || fargs->offset > msg_data_size)
		return -EINVAL; // invalid range
	
	fargs->buffer_size = MIN(fargs->buffer_size, msg_data_size - fargs->offset);
	
	if(fargs->buffer_size == 0)
		return -1; // EOF
	
	memcpy(fargs->buffer, msg_data + fargs->offset, fargs->buffer_size);
	
	zmq_msg_close(&zmq_msg);
	return 0;
} // }}}
static ssize_t data_zeromq_t_write(data_t *data, fastcall_write *fargs){ // {{{
	ssize_t                ret;
	void                  *msg_data;
	zmq_msg_t              zmq_msg;
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	if( (ret = zeromq_t_prepare(fdata)) < 0)
		return ret;
	
	if( zmq_msg_init_size(&zmq_msg, fargs->buffer_size) != 0)
		return -errno;
	
	msg_data = zmq_msg_data(&zmq_msg);
	memcpy(msg_data, fargs->buffer, fargs->buffer_size);
	
	if( zmq_send(fdata->zmq_socket, &zmq_msg, 0) != 0)
		return -errno;
	
	return 0;
} // }}}
static ssize_t data_zeromq_t_transfer(data_t *data, fastcall_transfer *fargs){ // {{{
	ssize_t                ret;
	void                  *msg_data;
	size_t                 msg_data_size;
	zmq_msg_t              zmq_msg;
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	if( (ret = zeromq_t_prepare(fdata)) < 0)
		return ret;
	
	if( zmq_msg_init(&zmq_msg) != 0)
		return error("zmq_msg_init failed");
	
	if( zmq_recv(fdata->zmq_socket, &zmq_msg, 0) != 0)
		return -errno;
	
	msg_data      = zmq_msg_data(&zmq_msg);
	msg_data_size = zmq_msg_size(&zmq_msg);
	
	fastcall_write r_write = {
		{ 5, ACTION_WRITE },
		0,
		msg_data,
		msg_data_size
	};
	ret = data_query(fargs->dest, &r_write);
	
	zmq_msg_close(&zmq_msg);
	return ret;
} // }}}
static ssize_t data_zeromq_t_push(data_t *data, fastcall_push *fargs){ // {{{
	ssize_t                ret;
	data_t                 freeme, *freehint;
	void                  *msg_data;
	uintmax_t              msg_size;
	zmq_msg_t              zmq_msg;
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	if( (ret = zeromq_t_prepare(fdata)) < 0)
		return ret;
	
	// convert data to continious memory space
	switch( (ret = data_get_continious(fargs->data, &freeme, &msg_data, &msg_size)) < 0){
		case 0: // use old data, it is ok!
			freehint = fargs->data;
			break;
		case 1: // use converted data, and free old one
			freehint = &freeme;
			data_free(fargs->data);
			break;
		default:
			return ret;
	}
	if( (freehint = memdup(freehint, sizeof(data_t))) == NULL){
		data_free(&freeme);
		return -ENOMEM;
	}
	
	if(zmq_msg_init_data(&zmq_msg, msg_data, msg_size, &zeromq_t_msg_free, freehint) != 0)
		return -errno;
	
	if( zmq_send(fdata->zmq_socket, &zmq_msg, 0) != 0)
		return -errno;
	
	return 0;
} // }}}
static ssize_t data_zeromq_t_pop(data_t *data, fastcall_pop *fargs){ // {{{
	ssize_t                ret;
	zmq_msg_t             *zmq_msg;
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	if( (ret = zeromq_t_prepare(fdata)) < 0)
		return ret;
	
	if( (zmq_msg = malloc(sizeof(zmq_msg_t))) == NULL)
		return -ENOMEM;
	
	if( zmq_msg_init(zmq_msg) != 0)
		return error("zmq_msg_init failed");
	
	if( zmq_recv(fdata->zmq_socket, zmq_msg, 0) != 0)
		return -errno;
	
	data_t                 d_zmq_msg         = DATA_PTR_ZEROMQ_MSGT(zmq_msg);
	memcpy(fargs->data, &d_zmq_msg, sizeof(data_t));
	return 0;
} // }}}
static ssize_t data_zeromq_t_enum(data_t *data, fastcall_enum *fargs){ // {{{
	ssize_t                ret;
	data_t                 item;
	
	while(1){
		fastcall_pop  r_pop  = { { 3, ACTION_POP  }, &item };
		if( (ret = data_zeromq_t_pop(data, &r_pop)) < 0)
			break;
		
		fastcall_push r_push = { { 3, ACTION_PUSH }, &item };
		if( (ret = data_query(fargs->dest, &r_push)) < 0)
			break;
	}
	return ret;
} // }}}

data_proto_t zmq_proto = {
	.type_str               = ZEROMQT_NAME,
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]     = (f_data_func)&data_zeromq_t_convert_to,
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_zeromq_t_convert_from,
		[ACTION_FREE]           = (f_data_func)&data_zeromq_t_free,
		[ACTION_READ]           = (f_data_func)&data_zeromq_t_read,
		[ACTION_WRITE]          = (f_data_func)&data_zeromq_t_write,
		[ACTION_TRANSFER]       = (f_data_func)&data_zeromq_t_transfer,
		[ACTION_PUSH]           = (f_data_func)&data_zeromq_t_push,
		[ACTION_POP]            = (f_data_func)&data_zeromq_t_pop,
		[ACTION_ENUM]           = (f_data_func)&data_zeromq_t_enum,
	}
};

static ssize_t data_zeromq_msg_t_getdataptr(data_t *data, fastcall_getdataptr *fargs){ // {{{
	zmq_msg_t             *fdata             = (zmq_msg_t *)data->ptr;
	
	fargs->ptr = zmq_msg_data(fdata);
	return 0;
} // }}}
static ssize_t data_zeromq_msg_t_length(data_t *data, fastcall_length *fargs){ // {{{
	zmq_msg_t             *fdata             = (zmq_msg_t *)data->ptr;
	
	fargs->length = zmq_msg_size(fdata);
	return 0;
} // }}}
static ssize_t data_zeromq_msg_t_convert_to(data_t *data, fastcall_convert_to *fargs){ // {{{
	zmq_msg_t             *fdata             = (zmq_msg_t *)data->ptr;
	
	fastcall_write r_write = {
		{ 5, ACTION_WRITE },
		0,
		zmq_msg_data(fdata),
		zmq_msg_size(fdata)
	};
	return data_query(fargs->dest, &r_write);
} // }}}
static ssize_t data_zeromq_msg_t_free(data_t *data, fastcall_free *fargs){ // {{{
	zmq_msg_t             *fdata             = (zmq_msg_t *)data->ptr;
	
	zmq_msg_close(fdata);
	free(fdata);
	return 0;
} // }}}
static ssize_t data_zeromq_msg_t_nosys(data_t *data, fastcall_header *hargs){ // {{{
	return -ENOSYS;
} // }}}

data_proto_t zmq_msg_proto = {
	.type_str               = ZEROMQ_MSGT_NAME,
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_GETDATAPTR]     = (f_data_func)&data_zeromq_msg_t_getdataptr,
		[ACTION_LENGTH]         = (f_data_func)&data_zeromq_msg_t_length,
		[ACTION_CONVERT_TO]     = (f_data_func)&data_zeromq_msg_t_convert_to,
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_zeromq_msg_t_nosys,
		[ACTION_FREE]           = (f_data_func)&data_zeromq_msg_t_free,
		[ACTION_COPY]           = (f_data_func)&data_zeromq_msg_t_nosys,
	}
};

int main(void){
	errors_register((err_item *)&errs_list, &emodule);
	data_register(&zmq_proto);
	data_register(&zmq_msg_proto);
	return 0;
}
