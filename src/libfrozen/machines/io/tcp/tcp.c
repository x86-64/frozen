#include <libfrozen.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/**
 * @ingroup machine
 * @addtogroup mod_machine_tcp io/tcp
 */
/**
 * @ingroup mod_machine_tcp
 * @page page_tcp_info Description
 *
 * This machine maintain io from tcp sockets.
 */
/**
 * @ingroup mod_machine_tcp
 * @page page_tcp_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "io/tcp",
 *              addr                    = "0.0.0.0",         # address to listen on, default "127.0.0.1"
 *              port                    = (uint_t)'12345',   # port to listen
 *              machine                 = { ... }            # machine to create on new socket. use "io/tcp_child" to set place for new socket
 * }
 * @endcode
 */

#define EMODULE 42

typedef struct tcp_userdata {
	int                    socket;
	char                  *tcp_addr;
	uintmax_t              tcp_port;
	hash_t                *machine;
	
	uintmax_t              tcp_running;
} tcp_userdata;

typedef struct tcp_child_userdata {
	int                    socket;
	uintmax_t              primary;
} tcp_child_userdata;

uintmax_t                      tcp_global_inited = 0;
pthread_key_t                  tcp_socket_key;
pthread_key_t                  tcp_primary_key;

static ssize_t tcp_control_stop(machine_t *machine){ // {{{
	tcp_userdata          *userdata          = (tcp_userdata *)machine->userdata;
	
	close(userdata->socket);
	userdata->tcp_running = 0;
	return 0;
} // }}}
static ssize_t tcp_control_start(machine_t *machine){ // {{{
	struct sockaddr_in     addr;
	tcp_userdata          *userdata          = (tcp_userdata *)machine->userdata;
	
	if(userdata->tcp_running != 0)
		tcp_control_stop(machine);
	
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

static int tcp_init(machine_t *machine){ // {{{
	tcp_userdata        *userdata;
	
	if(tcp_global_inited == 0){
		if(pthread_key_create(&tcp_socket_key, NULL) != 0)
			return -EFAULT;
		if(pthread_key_create(&tcp_primary_key, NULL) != 0)
			return -EFAULT;
		
		tcp_global_inited = 1;
	}
	
	if((userdata = machine->userdata = calloc(1, sizeof(tcp_userdata))) == NULL)
		return error("calloc failed");
	
	userdata->tcp_addr = "127.0.0.1";
	return 0;
} // }}}
static int tcp_destroy(machine_t *machine){ // {{{
	tcp_userdata          *userdata          = (tcp_userdata *)machine->userdata;

	free(userdata);
	return 0;
} // }}}
static int tcp_configure(machine_t *machine, hash_t *config){ // {{{
	ssize_t                ret;
	tcp_userdata          *userdata          = (tcp_userdata *)machine->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, userdata->tcp_addr, config, HK(addr));
	hash_data_copy(ret, TYPE_UINTT,   userdata->tcp_port, config, HK(port));
	hash_data_copy(ret, TYPE_HASHT,   userdata->machine,  config, HK(machine));
	
	tcp_control_start(machine);
	return 0;
} // }}}

static ssize_t tcp_fast_handler(machine_t *machine, void *hargs){ // {{{
	int                    socket;
	tcp_userdata          *userdata          = (tcp_userdata *)machine->userdata;
	
	if(userdata->tcp_running == 0)
		return -EFAULT;
	
	if( (socket = accept(userdata->socket, NULL, 0)) < 0)
		return error("accept error");
	
	if(pthread_setspecific(tcp_socket_key, &socket) != 0)
		return error("pthread_setspecific error");
	
	if(pthread_setspecific(tcp_primary_key, NULL) != 0)
		return error("pthread_setspecific error");
	
	if(shop_new(userdata->machine) == NULL)
		return error("shop_new error");
	
	return 0;
} // }}}
static ssize_t tcp_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              action;
	
	hash_data_copy(ret, TYPE_UINTT, action, request, HK(action));
	
	fastcall_header r_fast = { 2, action };
	return tcp_fast_handler(machine, &r_fast);
} // }}}

machine_t tcp_proto = {
	.class          = "io/tcp",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = &tcp_init,
	.func_configure = &tcp_configure,
	.func_destroy   = &tcp_destroy,
	.machine_type_hash = {
		.func_handler  = &tcp_handler
	},
	.machine_type_fast = {
		.func_handler  = &tcp_fast_handler
	}
};

static int tcp_child_init(machine_t *machine){ // {{{
	int                   *socket;
	tcp_child_userdata    *userdata;
	
	if((userdata = machine->userdata = calloc(1, sizeof(tcp_child_userdata))) == NULL)
		return error("calloc failed");
	
	if( (socket = pthread_getspecific(tcp_socket_key)) == NULL)
		return -EFAULT;
	
	if(pthread_getspecific(tcp_primary_key) == NULL){
		if(pthread_setspecific(tcp_primary_key, userdata) != 0)
			return error("pthread_setspecific error");
		
		userdata->primary = 1;
	}
	
	userdata->socket = *socket;
	return 0;
} // }}}
static int tcp_child_destroy(machine_t *machine){ // {{{
	tcp_child_userdata    *userdata          = (tcp_child_userdata *)machine->userdata;
	
	if(userdata->primary == 1){
		close(userdata->socket);
	}
	free(userdata);
	return 0;
} // }}}

static ssize_t tcp_child_fast_handler(machine_t *machine, fastcall_header *hargs){ // {{{
	tcp_child_userdata    *userdata          = (tcp_child_userdata *)machine->userdata;
	data_t                 fdt               = DATA_FDT(userdata->socket, 0);
	
	return data_query(&fdt, hargs);
} // }}}
static ssize_t tcp_child_handler(machine_t *machine, request_t *request){ // {{{
	ssize_t                ret;
	uintmax_t              action;
	data_t                *buffer;
	tcp_child_userdata    *userdata          = (tcp_child_userdata *)machine->userdata;
	data_t                 fdt               = DATA_FDT(userdata->socket, 0);
	
	hash_data_copy(ret, TYPE_UINTT, action, request, HK(action));
	if(ret != 0)
		return -EINVAL;
	
	switch(action){
		case ACTION_READ:;
			buffer = hash_data_find(request, HK(buffer));
			fastcall_transfer r_transfer1 = { { 3, ACTION_TRANSFER }, buffer };
			return data_query(&fdt, &r_transfer1 );
			
		case ACTION_WRITE:;
			buffer = hash_data_find(request, HK(buffer));
			fastcall_transfer r_transfer2 = { { 3, ACTION_TRANSFER }, &fdt };
			return data_query(buffer, &r_transfer2 );
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}

machine_t tcp_child_proto = {
	.class          = "io/tcp_child",
	.supported_api  = API_HASH | API_FAST,
	.func_init      = &tcp_child_init,
	.func_destroy   = &tcp_child_destroy,
	.machine_type_hash = {
		.func_handler  = &tcp_child_handler
	},
	.machine_type_fast = {
		.func_handler  = (f_fast_func)&tcp_child_fast_handler
	}
};
