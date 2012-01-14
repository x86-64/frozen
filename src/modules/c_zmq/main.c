#include <libfrozen.h>
#include <zmq.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_zmq module/c_zmq
 */
/**
 * @ingroup mod_backend_zmq
 * @page page_zmq_info Description
 *
 * This module implement stubs to work with ZeroMQ.
 */
/**
 * @ingroup mod_backend_zmq
 * @page page_zmq_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "modules/c_zmq",
 *              type                    =                     # socket type to create, see man zmq_socket
 *                                        "req",
 *                                        "rep",
 *                                        "dealer",
 *                                        "router",
 *                                        "pub",
 *                                        "sub",
 *                                        "push",
 *                                        "pull",
 *                                        "pair",
 *              identity                = "ident",            # set socket identity
 *              bind                    = "tcp://",           # bind socket to            OR
 *              connect                 = "tcp://".           # connect socket to
 *
 *
 * }
 * @endcode
 */

#define EMODULE 101
#define ZMQ_NTHREADS        1

typedef struct zmq_userdata {
	void                  *zmq_socket;
} zmq_userdata;

void                          *zmq_ctx           = NULL;
uintmax_t                      zmq_backends      = 0;
pthread_mutex_t                zmq_backends_mtx  = PTHREAD_MUTEX_INITIALIZER;

static int zmq_string_to_type(char *type_str){ // {{{
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

static int zmqb_init(backend_t *backend){ // {{{
	ssize_t                ret               = 0;
	zmq_userdata          *userdata;
	
	// manage global zmq context
	pthread_mutex_lock(&zmq_backends_mtx);
		if(!zmq_ctx){
			if( (zmq_ctx = zmq_init(ZMQ_NTHREADS)) == NULL){
				ret = error("zmq_init failed");
				goto error;
			}
		}
		zmq_backends++;
	pthread_mutex_unlock(&zmq_backends_mtx);
	
	if((userdata = backend->userdata = calloc(1, sizeof(zmq_userdata))) == NULL)
		return error("calloc failed");
	
	return ret;

error:
	pthread_mutex_unlock(&zmq_backends_mtx);
	return ret;
} // }}}
static int zmqb_destroy(backend_t *backend){ // {{{
	zmq_userdata         *userdata          = (zmq_userdata *)backend->userdata;
	
	if(userdata->zmq_socket){
		zmq_close(userdata->zmq_socket);
	}
	
	// manage global zmq context
	pthread_mutex_lock(&zmq_backends_mtx);
		if(zmq_backends-- == 0){
			zmq_term(zmq_ctx);
			zmq_ctx = NULL;
		}
	pthread_mutex_unlock(&zmq_backends_mtx);
	
	free(userdata);
	return 0;
} // }}}
static int zmqb_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
	int                    zmq_socket_type;
	char                  *zmq_type_str      = NULL;
	char                  *zmq_opt_ident     = NULL;
	char                  *zmq_act_bind      = NULL;
	char                  *zmq_act_connect   = NULL;
	zmq_userdata          *userdata          = (zmq_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, zmq_type_str,    config, HK(type));
	hash_data_copy(ret, TYPE_STRINGT, zmq_act_bind,    config, HK(bind));
	hash_data_copy(ret, TYPE_STRINGT, zmq_act_connect, config, HK(connect));
	
	if( (zmq_socket_type = zmq_string_to_type(zmq_type_str)) == -1 )
		return error("invalid zmq type");
	
	if( (userdata->zmq_socket = zmq_socket(zmq_ctx, zmq_socket_type)) == NULL)
		return error("zmq_socket failed");
	
	//for(i; i<=sizeof(opts_array); i++){
		hash_data_copy(ret, TYPE_STRINGT, zmq_opt_ident,   config, HK(identity));
		if(zmq_opt_ident){
			if(zmq_setsockopt(userdata->zmq_socket, ZMQ_IDENTITY, zmq_opt_ident, strlen(zmq_opt_ident)) != 0)
				return error("zmq_setsockopt (identity) failed");
		}
	//}

	if(zmq_act_bind){
		if(zmq_bind(userdata->zmq_socket, zmq_act_bind) != 0)
			return error("zmq_bind failed");
	}else if(zmq_act_connect){
		if(zmq_connect(userdata->zmq_socket, zmq_act_connect) != 0)
			return error("zmq_connect failed");
	}
	return 0;
} // }}}

static ssize_t zmqb_fast_handler(backend_t *backend, fastcall_header *hargs){ // {{{
	void                  *data;
	size_t                 data_size;
	zmq_msg_t              zmq_msg;
	zmq_userdata          *userdata          = (zmq_userdata *)backend->userdata;
	
	switch(hargs->action){
		// case ACTION_TRANSFER TODO
		case ACTION_READ:;
			fastcall_read *rargs = (fastcall_read *)hargs;
			
			if( zmq_msg_init(&zmq_msg) != 0)
				return error("zmq_msg_init failed");
			
			if( zmq_recv(userdata->zmq_socket, &zmq_msg, 0) != 0)
				return -errno;
			
			data      = zmq_msg_data(&zmq_msg);
			data_size = zmq_msg_size(&zmq_msg);
			
			
			if(rargs->buffer == NULL || rargs->offset > data_size)
				return -EINVAL; // invalid range
			
			rargs->buffer_size = MIN(rargs->buffer_size, data_size - rargs->offset);
			
			if(rargs->buffer_size == 0)
				return -1; // EOF
			
			memcpy(rargs->buffer, data + rargs->offset, rargs->buffer_size);
			
			zmq_msg_close(&zmq_msg);
			return 0;
			
		case ACTION_WRITE:;
			fastcall_write *wargs = (fastcall_write *)hargs;
			
			if( zmq_msg_init_size(&zmq_msg, wargs->buffer_size) != 0)
				return -errno;
			
			data      = zmq_msg_data(&zmq_msg);
			memcpy(data, wargs->buffer, wargs->buffer_size); // TODO zero copy
			
			if( zmq_send(userdata->zmq_socket, &zmq_msg, 0) != 0)
				return -errno;
			
			return 0;
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
/*static ssize_t zmqb_io_t_multipart_handler(data_t *io_data, void *io_userdata, void *args){ // {{{
	void                  *data;
	zmq_msg_t              zmq_msg;
	backend_t             *backend           = (backend_t *)io_userdata;
	zmq_userdata          *userdata          = (zmq_userdata *)backend->userdata;
	fastcall_write        *wargs             = (fastcall_write *)args;
	
	if( zmq_msg_init_size(&zmq_msg, wargs->buffer_size) != 0)
		return -errno;
	
	data      = zmq_msg_data(&zmq_msg);
	memcpy(data, wargs->buffer, wargs->buffer_size); // TODO zero copy
	
	if( zmq_send(userdata->zmq_socket, &zmq_msg, ZMQ_SNDMORE) != 0)
		return -errno;
	
	return 0;
} // }}}*/
static ssize_t zmqb_io_t_handler(data_t *data, void *userdata, void *args){ // {{{
	return zmqb_fast_handler((backend_t *)userdata, args);
} // }}}
//static void zmq_buffer_free(void *data, void *hint){
	//buffer_free((buffer_t *)hint);
//}
static ssize_t zmqb_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              action;
	//zmq_msg_t              zmq_msg;
	data_t                *buffer;
	data_t                 zmq_iot           = DATA_IOT(backend, &zmqb_io_t_handler);
	//data_t                 zmq_iot_multi     = DATA_IOT(backend, &zmqb_io_t_multipart_handler);
	//zmq_userdata          *userdata          = (zmq_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_UINTT, action, request, HK(action));
	if(ret != 0)
		return -EINVAL;
	
	switch(action){
		case ACTION_READ:;
			buffer = hash_data_find(request, HK(buffer));
			fastcall_transfer r_transfer1 = { { 3, ACTION_TRANSFER }, buffer };
			return data_query(&zmq_iot, &r_transfer1 );
			
		case ACTION_WRITE:;
			//buffer_t           *flatbuffer;
			//void               *data;
			//uintmax_t           data_size;
			
			//flatbuffer = buffer_alloc();
			
			//data_t              d_flatbuffer = DATA_BUFFERT(flatbuffer);
			//buffer = hash_data_find(request, HK(buffer));
			//fastcall_transfer r_transfer2 = { { 3, ACTION_TRANSFER }, &d_flatbuffer };
			//ret = data_query(buffer, &r_transfer2 );
			
			//data      = buffer_defragment(flatbuffer);
			//data_size = buffer_get_size(flatbuffer);

			//if( zmq_msg_init_data(&zmq_msg, data, data_size, &zmq_buffer_free, NULL) != 0)
			//	return -errno;
			
			//if( zmq_send(userdata->zmq_socket, &zmq_msg, 0) != 0)
			//	return -errno;
			
			//return ret;
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}

static backend_t zmq_proto = {                 // NOTE need static or unique name
	.class          = "modules/c_zmq",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = &zmqb_init,
	.func_configure = &zmqb_configure,
	.func_destroy   = &zmqb_destroy,
	.backend_type_hash = {
		.func_handler = &zmqb_handler
	},
	.backend_type_fast = {
		.func_handler = (f_fast_func)&zmqb_fast_handler
	}
};

int main(void){
	class_register(&zmq_proto);
	return 0;
}
