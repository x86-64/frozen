#include <libfrozen.h>
#include <zmq.h>
#include <zeromq_t.h>

#define EMODULE 101
#define ZMQ_NTHREADS        1

typedef struct zeromq_t {
	void                  *zmq_socket;
} zeromq_t;

void                          *zmq_ctx           = NULL;
uintmax_t                      zmq_data          = 0;
pthread_mutex_t                zmq_data_mtx      = PTHREAD_MUTEX_INITIALIZER;

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
static ssize_t zeromq_t_destroy(zeromq_t *fdata){ // {{{
	if(fdata->zmq_socket)
		zmq_close(fdata->zmq_socket);
	
	// manage global zmq context
	pthread_mutex_lock(&zmq_data_mtx);
		if(zmq_data-- == 0){
			zmq_term(zmq_ctx);
			zmq_ctx = NULL;
		}
	pthread_mutex_unlock(&zmq_data_mtx);
	
	free(fdata);
	return 0;
} // }}}
static ssize_t zeromq_t_new(zeromq_t **pfdata, config_t *config){ // {{{
	ssize_t                ret;
	int                    zmq_socket_type;
	char                  *zmq_type_str      = NULL;
	char                  *zmq_opt_ident     = NULL;
	char                  *zmq_act_bind      = NULL;
	char                  *zmq_act_connect   = NULL;
	zeromq_t              *fdata;
	
	// manage global zmq context
	pthread_mutex_lock(&zmq_data_mtx);
		if(!zmq_ctx){
			if( (zmq_ctx = zmq_init(ZMQ_NTHREADS)) == NULL){
				pthread_mutex_unlock(&zmq_data_mtx);
				return error("zmq_init failed");
			}
		}
		zmq_data++;
	pthread_mutex_unlock(&zmq_data_mtx);
	
	if((fdata = calloc(1, sizeof(zeromq_t))) == NULL)
		return -ENOMEM;
	
	hash_data_get(ret, TYPE_STRINGT, zmq_type_str,    config, HK(type));
	hash_data_get(ret, TYPE_STRINGT, zmq_act_bind,    config, HK(bind));
	hash_data_get(ret, TYPE_STRINGT, zmq_act_connect, config, HK(connect));
	
	if( (zmq_socket_type = zeromq_t_string_to_type(zmq_type_str)) == -1 ){
		ret = error("invalid zmq type");
		goto error;
	}
	
	if( (fdata->zmq_socket = zmq_socket(zmq_ctx, zmq_socket_type)) == NULL){
		ret = error("zmq_socket failed");
		goto error;
	}
	
	//for(i; i<=sizeof(opts_array); i++){
		hash_data_get(ret, TYPE_STRINGT, zmq_opt_ident,   config, HK(identity));
		if(zmq_opt_ident){
			if(zmq_setsockopt(fdata->zmq_socket, ZMQ_IDENTITY, zmq_opt_ident, strlen(zmq_opt_ident)) != 0){
				ret = error("zmq_setsockopt (identity) failed");
				goto error;
			}
		}
	//}
	
	if(zmq_act_bind){
		if(zmq_bind(fdata->zmq_socket, zmq_act_bind) != 0){
			ret = error("zmq_bind failed");
			goto error;
		}
	}else if(zmq_act_connect){
		if(zmq_connect(fdata->zmq_socket, zmq_act_connect) != 0){
			ret = error("zmq_connect failed");
			goto error;
		}
	}
	
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
	data_register(&zmq_proto);
	return 0;
}
