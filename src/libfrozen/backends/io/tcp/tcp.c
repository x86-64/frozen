#include <libfrozen.h>
#include <dataproto.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_tcp io/tcp
 */
/**
 * @ingroup mod_backend_tcp
 * @page page_tcp_info Description
 *
 * This backend maintain io from tcp sockets.
 */
/**
 * @ingroup mod_backend_tcp
 * @page page_tcp_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "io/tcp",
 *              addr                    = "0.0.0.0",         # address to listen on, default "127.0.0.1"
 *              port                    = (uint_t)'12345',   # port to listen
 *              backend                 = { ... }            # backend to create on new socket. use "io/tcp_child" to set place for new socket
 * }
 * @endcode
 */

#define EMODULE 42

typedef struct tcp_userdata {
	int                    socket;
	char                  *tcp_addr;
	uintmax_t              tcp_port;
	hash_t                *backend;
	
	uintmax_t              tcp_running;
} tcp_userdata;

typedef struct tcp_child_userdata {
	int                    socket;
} tcp_child_userdata;

uintmax_t                      global_inited     = 0;
pthread_key_t                  tcp_childud_key;

static ssize_t tcp_control_stop(backend_t *backend){ // {{{
	tcp_userdata          *userdata          = (tcp_userdata *)backend->userdata;
	
	close(userdata->socket);
	userdata->tcp_running = 0;
	return 0;
} // }}}
static ssize_t tcp_control_start(backend_t *backend){ // {{{
	struct sockaddr_in     addr;
	tcp_userdata          *userdata          = (tcp_userdata *)backend->userdata;
	
	if(userdata->tcp_running != 0)
		tcp_control_stop(backend);
	
	if( (userdata->socket = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
		return error("socket error");
	
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = inet_addr(userdata->tcp_addr);
	addr.sin_port        = htons((uint16_t)userdata->tcp_port);
	
	if(bind(userdata->socket, (struct sockaddr *)&addr, sizeof(addr)) != 0)
		return error("bind error");
	
	if(listen(userdata->socket, 2) < 0)
		return error("listen error");
	
	userdata->tcp_running = 1;
	return 0;
} // }}}

static int tcp_init(backend_t *backend){ // {{{
	tcp_userdata        *userdata;
	
	if(global_inited == 0){
		if(pthread_key_create(&tcp_childud_key, NULL) != 0)
			return -EFAULT;
		
		global_inited = 1;
	}
	
	if((userdata = backend->userdata = calloc(1, sizeof(tcp_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->tcp_addr = "127.0.0.1";
	return 0;
} // }}}
static int tcp_destroy(backend_t *backend){ // {{{
	tcp_userdata          *userdata          = (tcp_userdata *)backend->userdata;

	free(userdata);
	return 0;
} // }}}
static int tcp_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	tcp_userdata          *userdata          = (tcp_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, userdata->tcp_addr, config, HK(addr));
	hash_data_copy(ret, TYPE_UINTT,   userdata->tcp_port, config, HK(port));
	hash_data_copy(ret, TYPE_HASHT,   userdata->backend,  config, HK(backend));
	
	tcp_control_start(backend);
	return 0;
} // }}}

static ssize_t tcp_fast_handler(backend_t *backend, void *hargs){ // {{{
	tcp_child_userdata    *child_userdata;
	tcp_userdata          *userdata          = (tcp_userdata *)backend->userdata;
	
	if(userdata->tcp_running == 0)
		return -EFAULT;
	
	if((child_userdata = calloc(1, sizeof(tcp_child_userdata))) == NULL)
		return error("calloc failed");
	
	if( (child_userdata->socket = accept(userdata->socket, NULL, 0)) < 0)
		return error("accept error");
	
	if(pthread_setspecific(tcp_childud_key, child_userdata) != 0)
		return error("pthread_setspecific error");
	
	if(backend_new(userdata->backend) == NULL)
		return error("backend_new error");
	
	return 0;
} // }}}
static ssize_t tcp_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              action;
	
	hash_data_copy(ret, TYPE_UINTT, action, request, HK(action));
	
	fastcall_header r_fast = { 2, action };
	return tcp_fast_handler(backend, &r_fast);
} // }}}

backend_t tcp_proto = {
	.class          = "io/tcp",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = &tcp_init,
	.func_configure = &tcp_configure,
	.func_destroy   = &tcp_destroy,
	.backend_type_hash = {
		.func_handler  = &tcp_handler
	},
	.backend_type_fast = {
		.func_handler  = &tcp_fast_handler
	}
};

static int tcp_child_init(backend_t *backend){ // {{{
	tcp_child_userdata    *userdata;
	
	if( (userdata = backend->userdata = pthread_getspecific(tcp_childud_key)) == NULL)
		return -EFAULT;
	
	return 0;
} // }}}
static int tcp_child_destroy(backend_t *backend){ // {{{
	tcp_child_userdata    *userdata          = (tcp_child_userdata *)backend->userdata;

	free(userdata);
	return 0;
} // }}}

static ssize_t tcp_child_fast_handler(backend_t *backend, fastcall_header *hargs){ // {{{
	tcp_child_userdata    *userdata          = (tcp_child_userdata *)backend->userdata;
	
	switch(hargs->action){
		case ACTION_READ:;
			fastcall_read *rargs = (fastcall_read *)hargs;
			
			if( (rargs->buffer_size = read(userdata->socket, rargs->buffer, rargs->buffer_size)) < 0){
				rargs->buffer_size = 0;
				return -errno;
			}
			return 0;
			
		case ACTION_WRITE:;
			fastcall_write *wargs = (fastcall_write *)hargs;
			
			if( (wargs->buffer_size = write(userdata->socket, wargs->buffer, wargs->buffer_size)) < 0){
				wargs->buffer_size = 0;
				return -errno;
			}
			return 0;
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t tcp_child_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              action;
	
	hash_data_copy(ret, TYPE_UINTT, action, request, HK(action));
	
	return -ENOSYS;
} // }}}

backend_t tcp_child_proto = {
	.class          = "io/tcp_child",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = &tcp_child_init,
	.func_destroy   = &tcp_child_destroy,
	.backend_type_hash = {
		.func_handler  = &tcp_child_handler
	},
	.backend_type_fast = {
		.func_handler  = (f_fast_func)&tcp_child_fast_handler
	}
};
