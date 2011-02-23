#ifndef IPC_SHMEM_H
#define IPC_SHMEM_H

ssize_t ipc_shmem_init    (ipc_t *ipc, config_t *config);
ssize_t ipc_shmem_destroy (ipc_t *ipc);
ssize_t ipc_shmem_query   (ipc_t *ipc, request_t *request);

#endif
