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
 *              identity                = (string_t)'ident',  # set socket identity
 *              bind                    = (string_t)'',       # bind socket to            OR
 *              connect                 = (string_t)''.       # connect socket to
 *
 *
 * }
 * @endcode
 */

#define EMODULE 101
#define ZMQ_NTHREADS        1

typedef struct zmq_userdata {
	void                  *zmq_socket;
	zmq_msg_t              zmq_recv_msg;
} zmq_userdata;

void                          *zmq_ctx           = NULL;
uintmax_t                      zmq_backends      = 0;
pthread_mutex_t                zmq_backends_mtx  = PTHREAD_MUTEx_INITIALIZER;

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

static int zmq_init(backend_t *backend){ // {{{
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
static int zmq_destroy(backend_t *backend){ // {{{
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
static int zmq_configure(backend_t *backend, config_t *config){ // {{{
	ssize_t                ret;
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
	
	if( zmq_msg_init(&userdata->zmq_recv_msg) != 0)
		return error("zmq_msg_init failed");
	
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

static ssize_t zmq_fast_handler(backend_t *backend, fastcall_header *hargs){ // {{{
	//zmq_userdata         *userdata = (zmq_userdata *)backend->userdata;
	
	return 0;
} // }}}
static ssize_t zmq_handler(backend_t *backend, request_t *request){ // {{{
	//zmq_userdata         *userdata = (zmq_userdata *)backend->userdata;
	
	return 0;
} // }}}

static backend_t zmq_proto = {                 // NOTE need static or unique name
	.class          = "modules/c_zmq",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = &zmq_init,
	.func_configure = &zmq_configure,
	.func_destroy   = &zmq_destroy,
	.backend_type_hash = {
		.func_handler = &zmq_handler
	},
	.backend_type_fast = {
		.func_handler = &zmq_fast_handler
	}
};

int main(void){
	class_register(&zmq_proto);
	return 0;
}
