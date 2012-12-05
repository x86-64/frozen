#include <libfrozen.h>
#include <zmq.h>
#include <zeromq_t.h>
#include <shop/pass/pass.h>

#include <errors_list.c>

#define ZMQ_NTHREADS        1

typedef struct zeromq_t {
	void                  *zmq_socket;
	hash_t                *config;
	uintmax_t              suspended;

	int                    zmq_type;                
	
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

typedef struct suspended_item {
	uintmax_t              time;
	data_t                 item;
} suspended_item;

list                           suspended         = LIST_INITIALIZER;
pthread_mutex_t                suspended_mtx     = PTHREAD_MUTEX_INITIALIZER;

static ssize_t suspended_suspend(data_t *item){ // {{{
	suspended_item        *descr;
	
	if( (descr = malloc(sizeof(suspended_item))) == NULL)
		return -ENOMEM;
	
	descr->time    = time(NULL);
	descr->item    = *item;
	
	pthread_mutex_lock(&suspended_mtx);
		
		list_add(&suspended, descr);
		
	pthread_mutex_unlock(&suspended_mtx);
	
	data_set_void(item);
	return 0;
} // }}}
static ssize_t suspended_raise(data_t *item){ // {{{
	ssize_t                ret;
	suspended_item        *curr;
	uintmax_t              ctime             = time(NULL);
	void                  *iter              = NULL;
	
	pthread_mutex_lock(&suspended_mtx);
		while( (curr = list_iter_next(&suspended, &iter)) != NULL){
			fastcall_compare r_compare = { { 4, ACTION_COMPARE }, &curr->item };
			if( (ret = data_query(item, &r_compare)) < 0)
				goto error;
			
			if(r_compare.result == 0)
				break;
			
			if(curr->time + 20 <= ctime){      // free expired data
				data_free(&curr->item);
				
				list_delete(&suspended, curr);
				free(curr);
			}
		}
		if(curr != NULL){
			ret = 0;
			data_free(item);
			*item = curr->item;
			
			list_delete(&suspended, curr);
			free(curr);
		}else{
			ret = -EINVAL;
		}
error:
	pthread_mutex_unlock(&suspended_mtx);
	return ret;
} // }}}

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

static int     zeromq_t_string_to_type(char *type_str){ // {{{
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
static int     zeromq_t_string_to_dev(char *type_str){ // {{{
	if(type_str != NULL){
		if(strcasecmp(type_str, "queue") == 0) return ZMQ_QUEUE;
		if(strcasecmp(type_str, "forwarder") == 0) return ZMQ_FORWARDER;
		if(strcasecmp(type_str, "streamer") == 0) return ZMQ_STREAMER;
	}
	return -1;
} // }}}
static ssize_t zeromq_t_error_to_ret(ssize_t ret, ssize_t def){ // {{{
	switch(ret){
		case EINVAL:           return error("The endpoint supplied is invalid");
		case EPROTONOSUPPORT:  return error("The requested transport protocol is not supported.");
		case ENOCOMPATPROTO:   return error("The requested transport protocol is not compatible with the socket type.");
		case EADDRINUSE:       return error("The requested address is already in use.");
		case EADDRNOTAVAIL:    return error("The requested address was not local.");
		case ENODEV:           return error("The requested address specifies a nonexistent interface.");
		case ETERM:            return error("The 0MQ context associated with the specified socket was terminated.");
		case ENOTSOCK:         return error("The provided socket was invalid.");
		case EMTHREAD:         return error("No I/O thread is available to accomplish the task.");
		case EINTR:            return error("The operation was interrupted by delivery of a signal.");
		default: break;
	}
	return def;
} // }}}

// get opt {{{
#define get_opt(_key,_func){                                                                                          \
	if( (opt = hash_data_find(fdata->config, _key)) != NULL){                                                     \
		if(data_make_flat(opt, FORMAT(native), &freeme, &opt_binary_ptr, &opt_binary_size) >= 0){              \
			if( (p = strndup(opt_binary_ptr, opt_binary_size)) != NULL){                                  \
				_func;                                                                                \
				free(p);                                                                              \
			}else{                                                                                        \
				ret = error("strndup failed");                                                        \
			}                                                                                             \
		}else{                                                                                                \
			ret = error("data_make_flat failed on zmq options");                                          \
		}                                                                                                     \
		data_free(&freeme);                                                                                   \
	}                                                                                                             \
}
// }}}

static void    zeromq_t_socket_destroy(zeromq_t *fdata){ // {{{
	if(fdata->zmq_socket)
		zmq_close(fdata->zmq_socket);		
	
	zeromq_t_global_decrement();
} // }}}
static ssize_t zeromq_t_socket_new(zeromq_t *fdata){ // {{{
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
	
	if( (ret = zeromq_t_global_increment()) < 0)
		return ret;
	
	get_opt(HK(type),
		do{
			fdata->zmq_type = zeromq_t_string_to_type(p);
			if( (fdata->zmq_socket = zmq_socket(zmq_ctx, fdata->zmq_type)) != NULL){
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
			if(data_make_flat(opt, FORMAT(native), &freeme, &opt_binary_ptr, &opt_binary_size) >= 0){
				if(zmq_setsockopt(fdata->zmq_socket, c->option, opt_binary_ptr, opt_binary_size) != 0){
					ret = error("zmq_setsockopt binary failed");
				}
			}else{
				ret = error("data_make_flat failed on zmq options");
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
			if(zmq_bind(fdata->zmq_socket, p) == 0){
				ready = 2;
			}else{
				ret = zeromq_t_error_to_ret(errno, error("zmq_bind failed"));
			}
		}while(0)
	);
	get_opt(HK(connect),
		do{
			if(zmq_connect(fdata->zmq_socket, p) == 0){
				ready = 2;
			}else{
				ret = zeromq_t_error_to_ret(errno, error("zmq_connect failed"));
			}
		}while(0)
	);
	if(ret != 0)
		goto error;
	if(ready != 2){
		ret = error("no bind nor connect supplied in zmq options");
		goto error;
	}
	
	return 0;

error:
	zeromq_t_socket_destroy(fdata);
	return ret;
} // }}}

static void    zeromq_t_socket_susp_destroy(data_t *data){ // {{{
	zeromq_t              *fdata             = (zeromq_t *)data->ptr; 
	
	if(fdata->suspended == 0){
		fdata->suspended = 1;
		suspended_suspend(data);
		return;
	}
	zeromq_t_socket_destroy(fdata);
} // }}}
static ssize_t zeromq_t_socket_susp_new(data_t *data){ // {{{
	if(suspended_raise(data) >= 0){
		((zeromq_t *)(data->ptr))->suspended = 0;
		return 0;
	}
	
	zeromq_t              *fdata             = (zeromq_t *)data->ptr; 
	return zeromq_t_socket_new(fdata);
} // }}}

static void    zeromq_t_socket_susp_lazy_destroy(data_t *data){ // {{{
	zeromq_t              *fdata             = (zeromq_t *)data->ptr; 
	
	if(fdata->zmq_socket != NULL) // if( socket_created )
		zeromq_t_socket_susp_destroy(data);
} // }}}
static ssize_t zeromq_t_socket_susp_lazy_new(data_t *data){ // {{{
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	if(fdata->zmq_socket == NULL)
		return zeromq_t_socket_susp_new(data);
	
	return 0;
} // }}}

static void    zeromq_t_destroy(data_t *data){ // {{{
	if(data->ptr == NULL)
		return;
	
	zeromq_t_socket_susp_lazy_destroy(data);
	
	if(data->type == TYPE_VOIDT) // consumed by suspended mod
		return;
	
	zeromq_t              *fdata             = (zeromq_t *)data->ptr; 
	if(fdata->config)
		hash_free(fdata->config);
	
	free(fdata);
	data_set_void(data);
} // }}}
static ssize_t zeromq_t_prepare(data_t *data){ // {{{
	return zeromq_t_socket_susp_lazy_new(data);
} // }}}
static ssize_t zeromq_t_new(data_t *data, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              lazy              = 0;
	zeromq_t              *fdata;
	
	if((data->ptr = fdata = calloc(1, sizeof(zeromq_t))) == NULL)
		return -ENOMEM;
	
	fdata->config    = config;
	fdata->suspended = 0;
	
	hash_data_get(ret, TYPE_UINTT, lazy, fdata->config, HK(lazy));
	if(lazy != 0)
		return 0;
	
	return zeromq_t_prepare(data);
} // }}}

static void    zeromq_t_msg_free(void *data, void *hint){ // {{{
	if(hint){
		data_free( &(((data_t *)hint)[0]) ); // free open view on data
		data_free( &(((data_t *)hint)[1]) ); // free data holder
		free(hint);
	}
} // }}}

static ssize_t data_zeromq_t_compare(data_t *data, fastcall_compare *fargs){ // {{{
	ssize_t                ret;
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	// compare data2 with data1's hash. If data2 is zeromq socket, if would compare data1's hash with data2's hash
	
	data_t           d_config  = DATA_PTR_HASHT(fdata->config);
	fastcall_compare r_compare = { { 4, ACTION_COMPARE }, &d_config };
	ret = data_query(fargs->data2, &r_compare);
	
	fargs->result = r_compare.result;
	return ret;
} // }}}
static ssize_t data_zeromq_t_convert_to(data_t *data, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	void                  *msg_data;
	size_t                 msg_data_size;
	zmq_msg_t              zmq_msg;
	zeromq_t              *fdata             = (zeromq_t *)data->ptr;
	
	switch(fargs->format){
		case FORMAT(packed):;
			data_t             packed           = DATA_PTR_HASHT(fdata->config);
			return data_query(&packed, fargs);
			
		case FORMAT(native):;
			if( (ret = zeromq_t_prepare(data)) < 0)
				return ret;
			
			fdata = data->ptr;

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
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_zeromq_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(data->ptr != NULL)
		return 0;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t            *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0 || config == NULL)
				return -EINVAL;
			
			if( (config = hash_copy(config)) == NULL)
				return -ENOMEM;
			
			return zeromq_t_new(data, config);
			
		case FORMAT(packed):;
			data_t             packed           = DATA_PTR_HASHT(NULL); // don't free result, used internally 
			
			if( (ret = data_query(&packed, fargs) < 0))
				return ret;
			
			return zeromq_t_new(data, (hash_t *)packed.ptr);

		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_zeromq_t_free(data_t *data, fastcall_free *fargs){ // {{{
	zeromq_t_destroy(data);
	return 0;
} // }}}
static ssize_t data_zeromq_t_read(data_t *data, fastcall_read *fargs){ // {{{
	ssize_t                ret;
	void                  *msg_data;
	size_t                 msg_data_size;
	zmq_msg_t              zmq_msg;
	zeromq_t              *fdata;
	
	if( (ret = zeromq_t_prepare(data)) < 0)
		return ret;
	
	fdata = data->ptr;
	
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
	zeromq_t              *fdata;
	
	if( (ret = zeromq_t_prepare(data)) < 0)
		return ret;
	
	fdata = data->ptr;

	if( zmq_msg_init_size(&zmq_msg, fargs->buffer_size) != 0)
		return -errno;
	
	msg_data = zmq_msg_data(&zmq_msg);
	memcpy(msg_data, fargs->buffer, fargs->buffer_size);
	
	if( zmq_send(fdata->zmq_socket, &zmq_msg, 0) != 0)
		return -errno;
	
	return 0;
} // }}}
static ssize_t data_zeromq_t_push(data_t *data, fastcall_push *fargs){ // {{{
	ssize_t                ret;
	data_t                 freeme;
	data_t                *freehint          = NULL;
	void                  *msg_data          = NULL;
	uintmax_t              msg_size          = 0;
	zmq_msg_t              zmq_msg;
	zeromq_t              *fdata;
	
	if( (ret = zeromq_t_prepare(data)) < 0)
		return ret;
	
	fdata = data->ptr;

	if(fargs->data){
		// convert data to continious memory space
		if( (ret = data_make_flat(fargs->data, FORMAT(packed), &freeme, &msg_data, &msg_size)) < 0)
			return ret;
		
		if( (freehint = malloc(2 * sizeof(data_t))) == NULL){
			data_free(&freeme);
			return -ENOMEM;
		}
		freehint[0] = freeme;
		freehint[1] = *fargs->data;
		
		data_set_void(fargs->data);
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
	zeromq_t              *fdata;
	
	if( (ret = zeromq_t_prepare(data)) < 0)
		return ret;
	
	fdata = data->ptr;

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
		
		if(zmq_msg_size(item.ptr) == 0){
			data_free(&item);
			break;
		}
		
		fastcall_push r_push = { { 3, ACTION_PUSH }, &item };
		if( (ret = data_query(fargs->dest, &r_push)) < 0)
			break;
	}
	fastcall_push r_push = { { 3, ACTION_PUSH }, NULL }; // EOF
	data_query(fargs->dest, &r_push);
	return ret;
} // }}}
data_proto_t zmq_proto = { // {{{
	.type_str               = ZEROMQT_NAME,
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_TO]     = (f_data_func)&data_zeromq_t_convert_to,
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_zeromq_t_convert_from,
		[ACTION_FREE]           = (f_data_func)&data_zeromq_t_free,
		[ACTION_READ]           = (f_data_func)&data_zeromq_t_read,
		[ACTION_WRITE]          = (f_data_func)&data_zeromq_t_write,
		[ACTION_PUSH]           = (f_data_func)&data_zeromq_t_push,
		[ACTION_POP]            = (f_data_func)&data_zeromq_t_pop,
		[ACTION_ENUM]           = (f_data_func)&data_zeromq_t_enum,
		[ACTION_COMPARE]        = (f_data_func)&data_zeromq_t_compare,
	}
}; // }}}

static ssize_t data_zeromq_msg_t_view(data_t *data, fastcall_view *fargs){ // {{{
	zmq_msg_t             *fdata             = (zmq_msg_t *)data->ptr;
	
	if(fdata){
		fargs->ptr    = zmq_msg_data(fdata);
		fargs->length = zmq_msg_size(fdata);
		data_set_void(&fargs->freeit);
		return 0;
	}
	return -EINVAL;
} // }}}
static ssize_t data_zeromq_msg_t_length(data_t *data, fastcall_length *fargs){ // {{{
	zmq_msg_t             *fdata             = (zmq_msg_t *)data->ptr;
	
	fargs->length = zmq_msg_size(fdata);
	return 0;
} // }}}
static ssize_t data_zeromq_msg_t_convert_to(data_t *data, fastcall_convert_to *fargs){ // {{{
	ssize_t                ret;
	zmq_msg_t             *fdata             = (zmq_msg_t *)data->ptr;
	
	fastcall_write r_write = {
		{ 5, ACTION_WRITE },
		0,
		zmq_msg_data(fdata),
		zmq_msg_size(fdata)
	};
	ret = data_query(fargs->dest, &r_write);
	
	if(fargs->header.nargs >= 5)
		fargs->transfered = r_write.buffer_size;
	
	return ret;
} // }}}
static ssize_t data_zeromq_msg_t_free(data_t *data, fastcall_free *fargs){ // {{{
	zmq_msg_t             *fdata             = (zmq_msg_t *)data->ptr;
	
	zmq_msg_close(fdata);
	free(fdata);
	data_set_void(data);
	return 0;
} // }}}
static ssize_t data_zeromq_msg_t_nosys(data_t *data, fastcall_header *hargs){ // {{{
	return -ENOSYS;
} // }}}
data_proto_t zmq_msg_proto = { // {{{
	.type_str               = ZEROMQ_MSGT_NAME,
	.api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_VIEW]           = (f_data_func)&data_zeromq_msg_t_view,
		[ACTION_LENGTH]         = (f_data_func)&data_zeromq_msg_t_length,
		[ACTION_CONVERT_TO]     = (f_data_func)&data_zeromq_msg_t_convert_to,
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_zeromq_msg_t_nosys,
		[ACTION_FREE]           = (f_data_func)&data_zeromq_msg_t_free,
	}
}; // }}}


typedef struct zeromq_proxy_t {
	data_t                 zmq_socket;
	
	pthread_t              thread;
	data_t                 slave;       // slave data for zeromq_proxy_t
} zeromq_proxy_t;

static void *  zeromq_proxy_t_thread(void *fdata){ // {{{
	ssize_t                ret;
	data_t                 d_proxy           = { type_zeromq_proxyt, fdata };
	
	while(1){
		fastcall_enum r_enum = { { 3, ACTION_ENUM }, NULL };
		if( (ret = data_query(&d_proxy, &r_enum)) < 0 )
			errors_log("zeromq_proxy_t server error: %d: %s\n", ret, errors_describe(ret));
	}
	return NULL;
} // }}}
static void    zeromq_proxy_t_destroy(data_t *data){ // {{{
	void *                 res;
	zeromq_proxy_t        *fdata             = (zeromq_proxy_t *)data->ptr; 
	
	if(data_is_void(&fdata->slave)){
		// client
		
	}else{
		// server
		pthread_cancel(fdata->thread);
		pthread_join(fdata->thread, &res);
		
		data_free(&fdata->slave);
	}
	
	data_free(&fdata->zmq_socket);
	
	free(fdata);
	data_set_void(data);
} // }}}
static ssize_t zeromq_proxy_t_new(data_t *data, config_t *config){ // {{{
	ssize_t                ret;
	zeromq_proxy_t        *fdata;
	
	if((data->ptr = fdata = calloc(1, sizeof(zeromq_proxy_t))) == NULL)
		return -ENOMEM;
	
	hash_holder_consume(ret, fdata->slave, config, HK(data));
	if(ret != 0){
		data_set_void(&fdata->slave);
	}
	
	fdata->zmq_socket.type = type_zeromqt;
	fdata->zmq_socket.ptr  = NULL;
	
	if( (ret = zeromq_t_new(&fdata->zmq_socket, config)) < 0){
		data_free(&fdata->slave);
		return ret;
	}
	
	if( data_is_void(&fdata->slave) ){
		// client
		
	}else{
		// server
		if(pthread_create(&fdata->thread, NULL, &zeromq_proxy_t_thread, fdata) != 0)
			ret = errorn(EFAULT);
	}
	
	return ret;
} // }}}

static ssize_t data_zeromq_proxy_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(data->ptr != NULL)
		return 0;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t            *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0 || config == NULL)
				return -EINVAL;
			
			return zeromq_proxy_t_new(data, config);
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_zeromq_proxy_t_free(data_t *data, fastcall_free *fargs){ // {{{
	zeromq_proxy_t_destroy(data);
	return 0;
} // }}}
static ssize_t data_zeromq_proxy_t_default(data_t *data, fastcall_header *hargs){ // {{{
	ssize_t                ret;
	zeromq_proxy_t        *fdata             = (zeromq_proxy_t *)data->ptr;
	data_t                 d_input           = DATA_RAWT_EMPTY();
	data_t                 d_output          = DATA_RAWT_EMPTY();
	
	if(data_is_void(&fdata->slave)){
		// client side
		
		// 1. pack fastcall
		data_t                 d_fastcall        = DATA_FASTCALLT(&ret, hargs);
		fastcall_convert_to    r_convert_to      = { { 4, ACTION_CONVERT_TO }, &d_input, FORMAT(input) };
		if( (ret = data_query(&d_fastcall, &r_convert_to)) < 0)
			return ret;
		
		// 2. send it to server
		fastcall_push r_push = { { 4, ACTION_PUSH }, &d_input };
		if( (ret = data_zeromq_t_push(&fdata->zmq_socket, &r_push)) < 0)
			return ret;
		
		// 3. fetch result from server
		fastcall_pop r_pop = { { 4, ACTION_POP }, &d_output };
		if( (ret = data_zeromq_t_pop(&fdata->zmq_socket, &r_pop)) < 0)
			return ret;
		
		// 4. unpack fastcall
		fastcall_convert_from r_convert_from = { { 4, ACTION_CONVERT_FROM }, &d_output, FORMAT(output) };
		if( (ret = data_query(&d_fastcall, &r_convert_from)) < 0)
			return ret;
		
		data_free(&d_fastcall);
		data_free(&d_output);
		ret = 0;
	}else{
		// server side
		// 1. fetch packed fastcall
		fastcall_pop r_pop = { { 4, ACTION_POP }, &d_input };
		if( (ret = data_zeromq_t_pop(&fdata->zmq_socket, &r_pop)) < 0)
			return ret;
		
		// 2. unpack
		fastcall_header        zargs[FASTCALL_NARGS_LIMIT];
		data_t                 d_fastcall        = DATA_FASTCALLT(&ret, zargs);
		zargs[0].nargs = FASTCALL_NARGS_LIMIT;
		
		fastcall_convert_from  r_convert_from    = { { 4, ACTION_CONVERT_FROM }, &d_input, FORMAT(input) };
		if( (ret = data_query(&d_fastcall, &r_convert_from)) < 0)
			return ret;
		
		// 3. make a query
		ret = data_query(&fdata->slave, &zargs);
		
		// 4. pack fastcall
		fastcall_convert_to    r_convert_to      = { { 4, ACTION_CONVERT_TO }, &d_output, FORMAT(output) };
		if( (ret = data_query(&d_fastcall, &r_convert_to)) < 0)
			return ret;
		
		// 5. reply with packed fastcall
		fastcall_push r_push = { { 4, ACTION_PUSH }, &d_output };
		if( (ret = data_zeromq_t_push(&fdata->zmq_socket, &r_push)) < 0)
			return ret;
		
		data_free(&d_fastcall);
	}
	data_free(&d_input);
	return ret;
} // }}}
data_proto_t zmq_proxy_proto = { // {{{
	.type_str               = ZEROMQ_PROXYT_NAME,
	.api_type               = API_HANDLERS,
	.handler_default        = (f_data_func)&data_zeromq_proxy_t_default,
	.handlers               = {
		// use zeromq_t as base type
		[ACTION_CONVERT_FROM]   = (f_data_func)&data_zeromq_proxy_t_convert_from,
		[ACTION_FREE]           = (f_data_func)&data_zeromq_proxy_t_free,
		
	}
}; // }}}


typedef struct zeromq_userdata {
	data_t                 socket;
	data_t                 shop;
	hashkey_t              hk_input;
	hashkey_t              hk_output;
	machine_t             *zmq_end;
	uintmax_t              force_convert;
} zeromq_userdata;

static ssize_t zmq_end_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	data_t                *socket;
	data_t                *buffer;
	zeromq_userdata       *userdata          = (zeromq_userdata *)machine->userdata;
	
	data_realholder(ret, &userdata->socket, socket);
	if(ret < 0)
		return ret;
	
	if(socket->type != TYPE_ZEROMQT)
		return -EINVAL;
	
	if( (buffer = hash_data_find(request, userdata->hk_input)) == NULL )
		return -EINVAL;
	
	fastcall_push r_push = { { 3, ACTION_PUSH }, buffer };
	if( (ret = data_query(socket, &r_push)) < 0)
		return ret;
	
	return 0;
} // }}}
machine_t zmq_end_proto = { // {{{
	.supported_api  = API_HASH,
	.machine_type_hash = {
		.func_handler = &zmq_end_handler
	}
}; // }}}

static ssize_t zeromq_init(machine_t *machine){ // {{{
	zeromq_userdata       *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(zeromq_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->hk_input  = HK(buffer);
	userdata->hk_output = HK(buffer);
	userdata->force_convert = 0;
	
	if( (userdata->zmq_end = memdup(&zmq_end_proto, sizeof(machine_t))) == NULL){
		free(userdata);
		return error("malloc failed");
	}
	userdata->zmq_end->userdata = userdata;
	return 0;
} // }}}
static ssize_t zeromq_destroy(machine_t *machine){ // {{{
	zeromq_userdata       *userdata          = (zeromq_userdata *)machine->userdata;
	
	data_free(&userdata->socket);
	free(userdata->zmq_end);
	free(userdata);
	return 0;
} // }}}
static ssize_t zeromq_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	zeromq_userdata       *userdata          = (zeromq_userdata *)machine->userdata;
	
	hash_data_get(ret, TYPE_HASHKEYT, userdata->hk_input,  config, HK(input));
	hash_data_get(ret, TYPE_HASHKEYT, userdata->hk_output, config, HK(output));
	hash_data_get(ret, TYPE_UINTT,    userdata->force_convert, config, HK(convert));
	hash_holder_consume(ret, userdata->shop,    config, HK(shop));
	hash_holder_consume(ret, userdata->socket,  config, HK(socket));
	if(ret != 0)
		return error("socket parameter not supplied");
	
	return 0;
} // }}}

static ssize_t zeromq_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	data_t                 item;
	data_t                 input_convert     = DATA_VOID;
	data_t                *input;
	data_t                *socket;
	zeromq_t              *socket_fdata;
	zeromq_userdata       *userdata          = (zeromq_userdata *)machine->userdata;
	
	request_enter_context(request);
	
		data_realholder(ret, &userdata->socket, socket);
		if(ret < 0)
			return ret;
	
	request_leave_context();
	if(ret < 0)
		return ret;
	
	if(socket->type != TYPE_ZEROMQT)
		return -EINVAL;
	
	if((ret = zeromq_t_prepare(socket)) < 0)
		return ret;
	
	socket_fdata = socket->ptr;

	switch(socket_fdata->zmq_type){
		case ZMQ_REQ:;
		case ZMQ_PUB:;
		case ZMQ_PUSH:;
			if( (input = hash_data_find(request, userdata->hk_input)) == NULL )
				return -EINVAL;
			
			if(userdata->force_convert != 0){
				data_raw_t_empty(&input_convert);
				
				fastcall_convert_to r_convert = { { 4, ACTION_CONVERT_TO }, &input_convert, FORMAT(packed) };
				if( (ret = data_query(input, &r_convert)) < 0)
					return ret;
				
				input = &input_convert;
			}
			break;
		
		default:
			break;
	};

	switch(socket_fdata->zmq_type){
		case ZMQ_REQ:;
			fastcall_push r_req_push = { { 3, ACTION_PUSH }, input };
			if( (ret = data_query(socket, &r_req_push)) < 0)          // input buffer consumed here
				goto exit;
			
			fastcall_pop  r_req_pop  = { { 3, ACTION_POP  }, &item };
			if( (ret = data_query(socket, &r_req_pop)) < 0)           // new message allocated here
				goto exit;
			
			request_t r_req_next[] = {
				{ userdata->hk_output, item                              },
				hash_inline(request),
				hash_end
			};
			ret = machine_pass(machine, r_req_next);
			data_free(&r_req_next[0].data);                                   // new message free'd here, if not consumed
			goto exit;
			
		case ZMQ_REP:;
			fastcall_pop  r_rep_pop  = { { 3, ACTION_POP  }, &item };
			if( (ret = data_query(socket, &r_rep_pop)) < 0)           // message allocated here
				return ret;
			
			stack_call(userdata->zmq_end);
			
				request_t r_next[] = {
					{ userdata->hk_output,  item                             },
					hash_inline(request),
					hash_end
				};
				fastcall_query r_query = { { 3, ACTION_QUERY }, r_next };
				ret = data_query(&userdata->shop, &r_query);
			
			stack_clean();
			
			data_free(&r_next[0].data);                                       // message free'd here, if not consumed
			return ret;
		
		case ZMQ_PUB:;
		case ZMQ_PUSH:;
			fastcall_push r_pub_push = { { 3, ACTION_PUSH }, input };
			if( (ret = data_query(socket, &r_pub_push)) < 0)          // input buffer consumed here
				goto exit;
			
			break;
			
		case ZMQ_SUB:;
		case ZMQ_PULL:;
			fastcall_pop  r_sub_pop  = { { 3, ACTION_POP  }, &item };
			if( (ret = data_query(socket, &r_sub_pop)) < 0)           // new message allocated here
				return ret;
			
			request_t r_sub_next[] = {
				{ userdata->hk_output, item                              },
				hash_inline(request),
				hash_end
			};
			ret = machine_pass(machine, r_sub_next);
			data_free(&r_sub_next[0].data);                                   // new message free'd here, if not consumed
			return ret;
			
		default:
			
			break;
	};
	ret = machine_pass(machine, request);
exit:
	data_free(&input_convert);
	return ret;
} // }}}
static machine_t zmq_helper_proto = { // {{{
	.class          = "modules/zeromq",
	.supported_api  = API_HASH,
	.func_init      = &zeromq_init,
	.func_configure = &zeromq_configure,
	.func_destroy   = &zeromq_destroy,
	.machine_type_hash = {
		.func_handler = &zeromq_handler
	}
}; // }}}

typedef struct zeromq_dev_userdata {
	int                    device;
	data_t                 frontend;
	data_t                 backend;
} zeromq_dev_userdata;

static ssize_t zeromq_dev_init(machine_t *machine){ // {{{
	zeromq_dev_userdata   *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(zeromq_dev_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static ssize_t zeromq_dev_destroy(machine_t *machine){ // {{{
	zeromq_dev_userdata   *userdata          = (zeromq_dev_userdata *)machine->userdata;
	
	data_free(&userdata->frontend);
	data_free(&userdata->backend);
	free(userdata);
	return 0;
} // }}}
static ssize_t zeromq_dev_configure(machine_t *machine, config_t *config){ // {{{
	ssize_t                ret;
	char                  *device_str        = NULL;
	zeromq_dev_userdata   *userdata          = (zeromq_dev_userdata *)machine->userdata;
	
	hash_data_convert(ret, TYPE_STRINGT, device_str, config, HK(device));
	if(ret != 0)
		return error("device parameter not supplied");
		
	userdata->device = zeromq_t_string_to_dev(device_str);
	free(device_str);
	if(userdata->device == -1)
		return error("device string invalid");
	
	hash_holder_consume(ret, userdata->frontend,  config, HK(frontend));
	if(ret != 0)
		return error("frontend parameter not supplied");
	hash_holder_consume(ret, userdata->backend,  config, HK(backend));
	if(ret != 0)
		return error("backend parameter not supplied");
	
	return 0;
} // }}}

static ssize_t zeromq_dev_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret1, ret2;
	data_t                *frontend;
	data_t                *backend;
	zeromq_t              *frontend_fdata;
	zeromq_t              *backend_fdata;
	zeromq_dev_userdata   *userdata          = (zeromq_dev_userdata *)machine->userdata;
	
	request_enter_context(request);
		
		data_realholder(ret1, &userdata->frontend, frontend);
		data_realholder(ret2, &userdata->backend,  backend);
	
	request_leave_context();
	if(ret1 < 0) return ret1;
	if(ret2 < 0) return ret2;
	
	if(frontend->type != TYPE_ZEROMQT)
		return error("invalid frontend data supplied");
	if(backend->type != TYPE_ZEROMQT)
		return error("invalid backend data supplied");
	
	if(
		(ret1 = zeromq_t_prepare(frontend)) < 0 ||
		(ret1 = zeromq_t_prepare(backend)) < 0
	)
		return ret1;
	
	frontend_fdata = frontend->ptr;
	backend_fdata  = backend->ptr;
	
	zmq_device(userdata->device, frontend_fdata->zmq_socket, backend_fdata->zmq_socket);
	return 0;
} // }}}
static machine_t zmq_device_proto = { // {{{
	.class          = "modules/zeromq_device",
	.supported_api  = API_HASH,
	.func_init      = &zeromq_dev_init,
	.func_configure = &zeromq_dev_configure,
	.func_destroy   = &zeromq_dev_destroy,
	.machine_type_hash = {
		.func_handler = &zeromq_dev_handler
	}
}; // }}}

int main(void){
	errors_register((err_item *)&errs_list, &emodule);
	data_register(&zmq_proto);
	data_register(&zmq_msg_proto);
	data_register(&zmq_proxy_proto);
	class_register(&zmq_helper_proto);
	class_register(&zmq_device_proto);
	
	type_zeromqt       = datatype_t_getid_byname(ZEROMQT_NAME,       NULL);
	type_zeromq_msgt   = datatype_t_getid_byname(ZEROMQ_MSGT_NAME,   NULL);
	type_zeromq_proxyt = datatype_t_getid_byname(ZEROMQ_PROXYT_NAME, NULL);
	return 0;
}
