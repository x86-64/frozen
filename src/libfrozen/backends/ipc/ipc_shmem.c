#include <libfrozen.h>
#include <ipc.h>
#include <ipc_shmem.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>

#define EMODULE           9
#define NITEMS_DEFAULT    100
#define ITEM_SIZE_DEFAULT 1000

typedef enum block_status {
	STATUS_FREE      = 0,
	STATUS_WRITING   = 1,
	STATUS_WRITTEN   = 2,
	STATUS_EXECUTING = 3,
	STATUS_DONE      = 4
} block_status;

typedef struct ipc_shmem_header {
	size_t                 item_size;
	size_t                 nitems;
	
	sem_t                  sem_free;
	sem_t                  sem_written;
} ipc_shmem_header;

typedef struct ipc_shmem_block {
	volatile size_t        status;
	volatile ssize_t       query_ret;
	unsigned int           data;
	
	sem_t                  sem_done;
} ipc_shmem_block;

typedef struct ipc_shmem_userdata {
	size_t                 inited;
	ipc_role               role;
	
	ipc_shmem_header      *shmaddr;
	ipc_shmem_block       *shmblocks;
	void                  *shmdata;
	
	pthread_t              server_thr;
} ipc_shmem_userdata;

static ssize_t shmem_init(ipc_shmem_userdata *userdata){ // {{{
	size_t i;
	
	sem_init(&userdata->shmaddr->sem_written, 1, 0);
	sem_init(&userdata->shmaddr->sem_free,    1, userdata->shmaddr->nitems);
	for(i=0; i<userdata->shmaddr->nitems; i++){
		sem_init(&userdata->shmblocks[i].sem_done, 1, 0);
		userdata->shmblocks[i].data = i * userdata->shmaddr->item_size;
	}
	return 0;
} // }}}
static ssize_t shmem_destroy(ipc_shmem_userdata *userdata){ // {{{
	size_t i;
	
	sem_destroy(&userdata->shmaddr->sem_free);
	sem_destroy(&userdata->shmaddr->sem_written);
	for(i=0; i<userdata->shmaddr->nitems; i++){
		sem_destroy(&userdata->shmblocks[i].sem_done);
	}
	return 0;
} // }}}
static ssize_t           shmem_block_status(ipc_shmem_userdata *userdata, ipc_shmem_block *block, size_t old_status, size_t new_status){ // {{{
	if(__sync_bool_compare_and_swap(&block->status, old_status, new_status)){
		switch(new_status){
			case STATUS_FREE:    sem_post(&userdata->shmaddr->sem_free);    break;
			case STATUS_WRITTEN: sem_post(&userdata->shmaddr->sem_written); break;
			case STATUS_DONE:    sem_post(&block->sem_done);                break;
			default: break;
		}
		return 0;
	}
	return -EFAULT;
} // }}}
static ipc_shmem_block * shmem_get_block(ipc_shmem_userdata *userdata, size_t old_status, size_t new_status){ // {{{
	size_t           i;
	ipc_shmem_block *block;
	
	switch(old_status){
		case STATUS_FREE:    sem_wait(&userdata->shmaddr->sem_free);    break;
		case STATUS_WRITTEN: sem_wait(&userdata->shmaddr->sem_written); break;
		default: return NULL;
	}
	
	for(i=0; i<userdata->shmaddr->nitems; i++){
		block = &userdata->shmblocks[i];
		if(shmem_block_status(userdata, block, old_status, new_status) == 0)
			return block;
	}
	return NULL;
} // }}}

void *  ipc_shmem_listen  (void *ipc){ // {{{
	request_t          *request;
	ipc_shmem_block    *block;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)((ipc_t *)ipc)->userdata;
	
	while(1){
		if( (block = shmem_get_block(userdata, STATUS_WRITTEN, STATUS_EXECUTING)) == NULL)
			break;
		
		// run request
		hash_from_memory(&request, userdata->shmdata + block->data, userdata->shmaddr->item_size);
		block->query_ret = backend_pass(((ipc_t *)ipc)->backend, request);
		
		// update status
		if(shmem_block_status(userdata, block, STATUS_EXECUTING, STATUS_DONE) < 0)
			break;
	}
	error("ipc_shmem_listen dead\n");
	return NULL;
} // }}}
ssize_t ipc_shmem_init    (ipc_t *ipc, config_t *config){ // {{{
	ssize_t             ret;
	int                 shmid;
	DT_UINT32T            shmkey;
	DT_SIZET            nitems    = NITEMS_DEFAULT;
	DT_SIZET            item_size = ITEM_SIZE_DEFAULT;
	DT_SIZET            shmsize;
	DT_STRING           role_str;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	hash_data_copy(ret, TYPE_UINT32T,  shmkey,     config, HK(key)); if(ret != 0) return warning("no key supplied");
	hash_data_copy(ret, TYPE_STRING, role_str,   config, HK(role)); if(ret != 0) return warning("no role supplied");
	hash_data_copy(ret, TYPE_SIZET,  item_size,  config, HK(item_size));
	hash_data_copy(ret, TYPE_SIZET,  nitems,     config, HK(size));
	
	shmsize = nitems * sizeof(ipc_shmem_block) + nitems * item_size + sizeof(ipc_shmem_header); 
	
	if( (shmid = shmget(shmkey, shmsize, IPC_CREAT | 0666)) < 0)
		return error("shmget failed");
	
	if( (userdata->shmaddr = shmat(shmid, NULL, 0)) == (void *)-1)
		return error("shmat failed");
	
	if( (userdata->role = ipc_string_to_role(role_str)) == ROLE_INVALID)
		return error("invalid role supplied");
	
	userdata->shmblocks = (ipc_shmem_block *)((void *)userdata->shmaddr   + sizeof(ipc_shmem_header));
	userdata->shmdata   = (void *)           ((void *)userdata->shmblocks + nitems * sizeof(ipc_shmem_block));
	userdata->inited    = 1;
	
	if(userdata->role == ROLE_SERVER){
		userdata->shmaddr->item_size = item_size;
		userdata->shmaddr->nitems    = nitems;
		
		shmem_init(userdata);
		
		// start threads
		if(pthread_create(&userdata->server_thr, NULL, &ipc_shmem_listen, ipc) != 0)
			return error("pthread_create failed");
	}
	return 0;
} // }}}
ssize_t ipc_shmem_destroy (ipc_t *ipc){ // {{{
	void               *res;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	if(userdata->inited != 1)
		return 0;
	
	if(userdata->role == ROLE_SERVER){
		// remove threads
		if(pthread_cancel(userdata->server_thr) != 0)
			goto error;
		if(pthread_join(userdata->server_thr, &res) != 0)
			goto error;
		
		shmem_destroy(userdata);
	}
	
error:
	shmdt(userdata->shmaddr);
	userdata->inited = 0;
	return 0;
} // }}}
ssize_t ipc_shmem_query   (ipc_t *ipc, request_t *request){ // {{{
	ssize_t             ret;
	ipc_shmem_block    *block;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	if( (block = shmem_get_block(userdata, STATUS_FREE, STATUS_WRITING)) == NULL)
		return error("strange error");
	
	// write request
	hash_to_memory(request, userdata->shmdata + block->data, userdata->shmaddr->item_size);
	
	if( (ret = shmem_block_status(userdata, block, STATUS_WRITING, STATUS_WRITTEN)) < 0)
		return ret;
	
	// wait for answer
	sem_wait(&block->sem_done);
	
	// read request back
	hash_reread_from_memory(request, userdata->shmdata + block->data, userdata->shmaddr->item_size);
	ret = block->query_ret;
	
	if(shmem_block_status(userdata, block, STATUS_DONE, STATUS_FREE) < 0)
		return error("strange error 3");
	
	return ret;
} // }}}

