#define IPC_C
#include <libfrozen.h>
#include <backends/ipc/ipc.h>

typedef struct ipc_userdata {
	ipc_t            ipc;
	ipc_proto_t     *ipc_proto;
} ipc_userdata;

static ipc_proto_t *   ipc_string_to_proto(char *string){ // {{{
	if(string != NULL){
		if(strcmp(string, "shmem") == 0) return &ipc_protos[IPC_TYPE_SHMEM];
	}
	return NULL;
} // }}}

static int ipc_init(chain_t *chain){ // {{{
	if((chain->userdata = calloc(1, sizeof(ipc_userdata))) == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
static int ipc_destroy(chain_t *chain){ // {{{
	ipc_userdata *userdata = (ipc_userdata *)chain->userdata;
	
	free(userdata);
	return 0;
} // }}}
static int ipc_configure(chain_t *chain, hash_t *config){ // {{{
	ssize_t          ret;
	DT_STRING        ipc_type_str  = NULL;
	ipc_userdata    *userdata      = (ipc_userdata *)chain->userdata;
	
	hash_copy_data(ret, TYPE_STRING, ipc_type_str,  config, HK(type));
	
	if( (userdata->ipc_proto = ipc_string_to_proto(ipc_type_str)) == NULL)
		return_error(-EINVAL, "chain 'ipc' parameter 'type' invalid\n");
	
	if( userdata->ipc_proto->func_init(&userdata->ipc, config) < 0)
		return -EFAULT;
	
	return 0;
} // }}}

static ssize_t ipc_backend_query(chain_t *chain, request_t *request){ // {{{
	ssize_t       ret;
	buffer_t      buffer;
	ipc_userdata *userdata = (ipc_userdata *)chain->userdata;
	
	if(hash_to_buffer(request, &buffer) < 0)
		return -EFAULT;
	
	ret = userdata->ipc_proto->func_query(&buffer);
	
	return ret;
} // }}}

static chain_t chain_ipc = {
	"ipc",
	CHAIN_TYPE_CRWD,
	.func_init      = &ipc_init,
	.func_configure = &ipc_configure,
	.func_destroy   = &ipc_destroy,
	{{
		.func_create = &ipc_backend_query,
		.func_set    = &ipc_backend_query,
		.func_get    = &ipc_backend_query,
		.func_delete = &ipc_backend_query,
		.func_move   = &ipc_backend_query,
		.func_count  = &ipc_backend_query,
		.func_custom = &ipc_backend_query
	}}
};
CHAIN_REGISTER(chain_ipc)

