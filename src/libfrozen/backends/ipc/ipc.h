#ifndef IPC_H
#define IPC_H

typedef enum ipc_types {
	IPC_TYPE_INVALID     = 0,
	IPC_TYPE_SHMEM       = 1,
	//IPC_TYPE_UNIX_SOCKET = 2,
	//IPC_TYPE_TCP_SOCKET  = 3,
	//IPC_TYPE_UDP_SOCKET  = 4
} ipc_types;

typedef struct ipc_t             ipc_t;
typedef struct ipc_proto_t       ipc_proto_t;

typedef ssize_t (*ipc_func_init)  (ipc_t *ipc, config_t *config);
typedef ssize_t (*ipc_func_query) (buffer_t *buffer);

struct ipc_t {
	void                    *userdata;
};

struct ipc_proto_t {
	ipc_func_init            func_init;
	ipc_func_query           func_query;
};

#ifdef IPC_C
#include <backends/ipc/ipc_shmem.h>

static ipc_proto_t ipc_protos[] = {
	[IPC_TYPE_SHMEM] = {
	//	.func_init   =  &ipc_shmem_init,
	//	.func_query  =  &ipc_shmem_query
	}
};
//static size_t ipc_protos_size = sizeof(ipc_protos) / sizeof(ipc_protos[0]);

#endif
#endif
