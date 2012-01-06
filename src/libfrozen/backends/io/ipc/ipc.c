#define IPC_C
#include <libfrozen.h>
#include <ipc.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_ipc communication/ipc
 */
/**
 * @ingroup mod_backend_ipc
 * @page page_ipc_info Description
 *
 * This backend pass request to another local process.
 */
/**
 * @ingroup mod_backend_ipc
 * @page page_ipc_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "communication/ipc",
 *              type                    = <see types>              # mechanism to use for transport
 * }
 * @endcode
 */
#define EMODULE 8

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
       ipc_role        ipc_string_to_role(char *string){ // {{{
	if(string != NULL){
		if( strcmp(string, "server") == 0) return ROLE_SERVER;
		if( strcmp(string, "client") == 0) return ROLE_CLIENT;
	}
	return ROLE_INVALID;
} // }}}

static int ipc_init(backend_t *backend){ // {{{
	if((backend->userdata = calloc(1, sizeof(ipc_userdata))) == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int ipc_destroy(backend_t *backend){ // {{{
	ipc_userdata *userdata = (ipc_userdata *)backend->userdata;
	
	userdata->ipc_proto->func_destroy(&userdata->ipc);
	
	free(userdata);
	return 0;
} // }}}
static int ipc_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t          ret;
	char            *ipc_type_str  = NULL;
	ipc_userdata    *userdata      = (ipc_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, ipc_type_str,  config, HK(type));
	
	if( (userdata->ipc_proto = ipc_string_to_proto(ipc_type_str)) == NULL)
		return error("backend ipc parameter type invalid");
	
	if( (ret = userdata->ipc_proto->func_init(&userdata->ipc, config)) < 0)
		return ret;
	
	userdata->ipc.backend = backend;
	
	return 0;
} // }}}

static ssize_t ipc_backend_query(backend_t *backend, request_t *request){ // {{{
	ipc_userdata *userdata = (ipc_userdata *)backend->userdata;
	
	return userdata->ipc_proto->func_query(&userdata->ipc, request);
} // }}}

backend_t ipc_proto = {
	.class          = "io/ipc",
	.supported_api  = API_HASH,
	.func_init      = &ipc_init,
	.func_configure = &ipc_configure,
	.func_destroy   = &ipc_destroy,
	.backend_type_hash = {
		.func_handler = &ipc_backend_query,
	}
};


