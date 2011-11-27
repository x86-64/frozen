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
 * }
 * @endcode
 */

#define EMODULE 42

typedef struct tcp_userdata {
	int                    socket;
	char                  *tcp_addr;
	uintmax_t              tcp_port;
	
	uintmax_t              tcp_running;
} tcp_userdata;

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
	
	tcp_control_start(backend);
	return 0;
} // }}}

static ssize_t tcp_handler(backend_t *backend, request_t *request){ // {{{
	ssize_t                ret               = 0;
	//tcp_userdata          *userdata          = (tcp_userdata *)backend->userdata;
	
	return ret;
} // }}}

backend_t tcp_proto = {
	.class          = "io/tcp",
	.supported_api  = API_HASH,
	.func_init      = &tcp_init,
	.func_configure = &tcp_configure,
	.func_destroy   = &tcp_destroy,
	.backend_type_hash = {
		.func_handler  = &tcp_handler
	}
};
