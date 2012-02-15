#include <libfrozen.h>
#include <zmq.h>
#include <zeromq_t.h>

#include <errors_list.c>

#define ZMQ_NTHREADS        1

typedef struct zeromq_t {
	void                  *zmq_socket;
	hash_t                *config;
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

static ssize_t zeromq_t_destroy(zeromq_t *fdata){ // {{{
	if(fdata->zmq_socket)
		zmq_close(fdata->zmq_socket);
	
	if(fdata->config)
		hash_free(fdata->config);
	
	zeromq_t_global_decrement();
	
	free(fdata);
	return 0;
} // }}}
static ssize_t zeromq_t_new(zeromq_t **pfdata, config_t *config){ // {{{
	ssize_t                ret;
	zeromq_t              *fdata;
	
	if((fdata = calloc(1, sizeof(zeromq_t))) == NULL)
		return -ENOMEM;
	
	if( (ret = zeromq_t_global_increment()) < 0)
		return ret;
	
	fdata->config = hash_copy(config);
	
	if( (ret = zeromq_t_socket_from_config(fdata)) < 0)
		goto error;
	
	*pfdata = fdata;
	return 0;

error:
	zeromq_t_destroy(fdata);
	return ret;
} // }}}

static ssize_t data_zeromq_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->src == NULL)
		return -EINVAL; 
		
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t            *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return zeromq_t_new((zeromq_t **)&data->ptr, config);
			
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
	void                  *msg_data;
	size_t                 msg_data_size;
	zmq_msg_t              zmq_msg;
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	if(fdata == NULL)
		return -EINVAL;
	
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
	void                  *msg_data;
	zmq_msg_t              zmq_msg;
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
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
	
	if(fdata == NULL)
		return -EINVAL;
	
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

data_proto_t zmq_proto = {
	.type_str               = ZEROMQT_NAME,
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_zeromq_t_convert_from,
		[ACTION_FREE]           = (f_data_func)&data_zeromq_t_free,
		[ACTION_READ]           = (f_data_func)&data_zeromq_t_read,
		[ACTION_WRITE]          = (f_data_func)&data_zeromq_t_write,
		[ACTION_TRANSFER]       = (f_data_func)&data_zeromq_t_transfer,
	}
};

int main(void){
	errors_register((err_item *)&errs_list, &emodule);
	data_register(&zmq_proto);
	return 0;
}
