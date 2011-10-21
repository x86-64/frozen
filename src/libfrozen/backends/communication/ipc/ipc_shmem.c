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
	STATUS_EXEC_DONE = 4
} block_status;

typedef enum forced_states {
	FORCE_ASYNC,
	FORCE_SYNC,
	FORCE_NONE
} forced_states;

typedef struct ipc_shmem_header {
	size_t                 item_size;
	size_t                 nitems;
	
	sem_t                  sem_free;
	sem_t                  sem_written;
} ipc_shmem_header;

typedef struct ipc_shmem_block {
	volatile size_t        status;
	volatile size_t        return_result;
	unsigned int           data_rel_ptr;

	sem_t                  sem_done;
} ipc_shmem_block;

typedef struct ipc_shmem_userdata {
	size_t                 inited;
	ipc_role               role;
	forced_states          forced_state;
	hash_key_t             buffer;
	uintmax_t              noreread;

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
		userdata->shmblocks[i].data_rel_ptr = i * userdata->shmaddr->item_size;
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
static ssize_t shmem_block_status(ipc_shmem_userdata *userdata, ipc_shmem_block *block, size_t old_status, size_t new_status){ // {{{
	if(__sync_bool_compare_and_swap(&block->status, old_status, new_status)){
		switch(new_status){
			case STATUS_FREE:      sem_post(&userdata->shmaddr->sem_free);    break;
			case STATUS_WRITTEN:   sem_post(&userdata->shmaddr->sem_written); break;
			case STATUS_EXEC_DONE: sem_post(&block->sem_done);                break;
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
	ipc_shmem_block    *block;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)((ipc_t *)ipc)->userdata;
	
	while(1){
		if( (block = shmem_get_block(userdata, STATUS_WRITTEN, STATUS_EXECUTING)) == NULL)
			break;
		
		// run request
		request_t req[] = {
			{ userdata->buffer, DATA_RAW( userdata->shmdata + block->data_rel_ptr, userdata->shmaddr->item_size ) },
			hash_end
		};
		backend_pass(((ipc_t *)ipc)->backend, req);
		
		// update status. if client don't want read results - free block, overwise notify client
		if(shmem_block_status(userdata, block, STATUS_EXECUTING, 
			((block->return_result == 0) ? STATUS_FREE : STATUS_EXEC_DONE)
		) < 0)
			break;
	}
	error("ipc_shmem_listen dead\n");
	return NULL;
} // }}}
ssize_t ipc_shmem_init    (ipc_t *ipc, config_t *config){ // {{{
	ssize_t                ret;
	int                    shmid;
	uint32_t               shmkey;
	size_t                 shmsize;
	size_t                 nitems            = NITEMS_DEFAULT;
	size_t                 item_size         = ITEM_SIZE_DEFAULT;
	uintmax_t              f_async           = 0;
	uintmax_t              f_sync            = 0;
	char                  *role_str;
	ipc_shmem_userdata    *userdata          = (ipc_shmem_userdata *)ipc->userdata;
	
	userdata->buffer = HK(buffer);

	hash_data_copy(ret, TYPE_UINT32T, shmkey,     config, HK(key));  if(ret != 0) return warning("no key supplied");
	hash_data_copy(ret, TYPE_STRINGT, role_str,   config, HK(role)); if(ret != 0) return warning("no role supplied");
	hash_data_copy(ret, TYPE_SIZET,   item_size,  config, HK(item_size));
	hash_data_copy(ret, TYPE_SIZET,   nitems,     config, HK(size));
	hash_data_copy(ret, TYPE_UINTT,   f_async,    config, HK(force_async));
	hash_data_copy(ret, TYPE_UINTT,   f_sync,     config, HK(force_sync));
	hash_data_copy(ret, TYPE_HASHKEYT, userdata->buffer,   config, HK(buffer));
	hash_data_copy(ret, TYPE_UINTT,    userdata->noreread, config, HK(noreread));

	shmsize = nitems * sizeof(ipc_shmem_block) + nitems * item_size + sizeof(ipc_shmem_header); 
	
	if( (shmid = shmget(shmkey, shmsize, IPC_CREAT | 0666)) < 0)
		return error("shmget failed");
	
	if( (userdata->shmaddr = shmat(shmid, NULL, 0)) == (void *)-1)
		return error("shmat failed");
	
	if( (userdata->role = ipc_string_to_role(role_str)) == ROLE_INVALID)
		return error("invalid role supplied");
	
	if( (f_async != 0 && f_sync != 0) )
		return error("force_async with force_sync");
	
	userdata->shmblocks = (ipc_shmem_block *)((void *)userdata->shmaddr   + sizeof(ipc_shmem_header));
	userdata->shmdata   = (void *)           ((void *)userdata->shmblocks + nitems * sizeof(ipc_shmem_block));
	userdata->inited    = 1;
	
	userdata->forced_state = FORCE_NONE;
	if(f_async != 0) userdata->forced_state = FORCE_ASYNC;
	if(f_sync  != 0) userdata->forced_state = FORCE_SYNC;
	
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
	ssize_t                ret;
	uintmax_t              f_async           = 0;
	data_t                *buffer;
	ipc_shmem_block       *block;
	ipc_shmem_userdata    *userdata          = (ipc_shmem_userdata *)ipc->userdata;
	
	// check async state
	switch(userdata->forced_state){
		case FORCE_SYNC:  break; // already == 0
		case FORCE_ASYNC: f_async = 1; break;
		case FORCE_NONE:  hash_data_copy(ret, TYPE_UINTT, f_async, request, HK(async)); break;
	};
	
	buffer = hash_data_find(request, userdata->buffer);
	
	// send request
	if( (block = shmem_get_block(userdata, STATUS_FREE, STATUS_WRITING)) == NULL)
		return error("strange error");
	
	// write data to ipc memory
	data_t               d_ipcmem  = DATA_RAW(userdata->shmdata + block->data_rel_ptr, userdata->shmaddr->item_size);
	fastcall_convert_to  r_convert = { { 4, ACTION_CONVERT_TO }, &d_ipcmem, FORMAT_BINARY };
	if( (ret = data_query(buffer, &r_convert)) < 0)
		return error("cant write buffer to ipc memory");
	
	//block->size          = r_read.buffer_size;
	block->return_result = (f_async == 0) ? 1 : 0;
	
	if( (ret = shmem_block_status(userdata, block, STATUS_WRITING, STATUS_WRITTEN)) < 0)
		return ret;
	
	if(f_async == 0){ // synchronous request
		// wait for answer
		sem_wait(&block->sem_done);
		
		// read request back
		if(userdata->noreread == 0){
			fastcall_read r_write = { { 5, ACTION_WRITE }, 0, userdata->shmdata + block->data_rel_ptr, userdata->shmaddr->item_size };
			data_query(buffer, &r_write);
		}
		ret = -EEXIST;
		
		if(shmem_block_status(userdata, block, STATUS_EXEC_DONE, STATUS_FREE) < 0)
			return error("strange error 3");
	}else{
		ret = 0; // success
	}
	return ret;
} // }}}

