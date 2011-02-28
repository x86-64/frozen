#include <libfrozen.h>
#include <backends/ipc/ipc.h>
#include <backends/ipc/ipc_shmem.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>

#define ITEM_SIZE_DEFAULT 1000

typedef enum window_type {
	WND_WRITE  = 0,
	WND_READ   = 1,
	WND_RETURN = 2
} window_type;

typedef struct window_t {
	volatile size_t      end;
	volatile size_t      start;
} window_t;

typedef struct ipc_shmem_header {
	sem_t                  sem[2];
	window_t               windows[3];
} ipc_shmem_header;

typedef struct ipc_shmem_block {
	volatile ssize_t       query_ret;
	volatile size_t        done[3];
} ipc_shmem_block;

typedef struct ipc_shmem_userdata {
	size_t            inited;
	ipc_role          role;
	
	size_t            item_size;
	size_t            nitems;
	
	pthread_t         server_thr;
	
	ipc_shmem_header *shmaddr;
	ipc_shmem_block  *shmblocks;
	void             *shmdata;
} ipc_shmem_userdata;

typedef ssize_t (*func_callb) (ipc_t *ipc, ipc_shmem_block *block, void *data, size_t data_size, void *arg);
	
#define shmem_next(curr) ( (curr + 1 >= userdata->nitems) ? 0 : curr + 1)

static ssize_t shmem_init(ipc_shmem_userdata *userdata){ // {{{
	sem_init(&userdata->shmaddr->sem[WND_WRITE],  1, userdata->item_size);
	sem_init(&userdata->shmaddr->sem[WND_READ],   1, 0);
	return 0;
} // }}}
static ssize_t shmem_do(ipc_t *ipc, window_type windowt, func_callb callb, void *arg, uint32_t *p_blockid){ // {{{
	ssize_t             ret;
	uint32_t            blockid, blockid_next;
	window_type         windowt_dep;
	window_t           *window_curr, *window_deps;
	void               *block_data;
	ipc_shmem_block    *block;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	windowt_dep = (windowt == WND_RETURN) ? WND_READ : WND_WRITE;
	window_curr = &userdata->shmaddr->windows[windowt];
	window_deps = &userdata->shmaddr->windows[windowt_dep];
	
	// reserve block
	while(1){
		blockid      = window_curr->start;
		
		if(windowt == WND_WRITE){
			blockid_next = shmem_next(blockid);
			if(blockid_next == window_deps->end) goto sleep;
		}else{
			if(blockid      == window_deps->end) goto sleep;
			blockid_next = shmem_next(blockid);
		}
		
		if(__sync_bool_compare_and_swap(&window_curr->start, blockid, blockid_next))
			break;
		
		continue;
sleep:
		sem_wait(&userdata->shmaddr->sem[windowt_dep]);
	}
	
	*p_blockid = blockid;
	block      = &userdata->shmblocks[blockid];
	block_data =  userdata->shmdata + (blockid * userdata->item_size);
	
	// call callback
	ret = callb(ipc, block, block_data, userdata->item_size, arg);
	
	block->done[WND_WRITE]  = (WND_WRITE  == windowt) ? 1 : 0;
	block->done[WND_READ]   = (WND_READ   == windowt) ? 1 : 0;
	block->done[WND_RETURN] = (WND_RETURN == windowt) ? 1 : 0;
	
	while(1){
		blockid      = window_curr->end;
		block        = &userdata->shmblocks[blockid];
		
		if(!__sync_bool_compare_and_swap(&block->done[windowt], 1, 0))
			break;
		
		blockid_next = shmem_next(blockid);
		
		__sync_bool_compare_and_swap(&window_curr->end, blockid, blockid_next);
		
		if(windowt != WND_RETURN)
			sem_post(&userdata->shmaddr->sem[windowt]);
	}
	return ret;
} // }}}

ssize_t callb_write(ipc_t *ipc, ipc_shmem_block *block, void *data, size_t data_size, void *p_request){ // {{{
	hash_to_memory((request_t *)p_request, data, data_size);
	return 0;
} // }}}
ssize_t callb_read(ipc_t *ipc, ipc_shmem_block *block, void *data, size_t data_size, void *null){ // {{{
	ssize_t             ret;
	request_t          *request;
	
	hash_from_memory(&request, data, data_size);
	//request_t request[] = { { HK(action), DATA_INT32(ACTION_CRWD_CREATE) }, hash_end };
	ret = chain_next_query(ipc->chain, request);
	
	block->query_ret = ret;
	return ret;
} // }}}
ssize_t callb_return(ipc_t *ipc, ipc_shmem_block *block, void *data, size_t data_size, void *p_request){ // {{{
	hash_reread_from_memory((request_t *)p_request, data, data_size);
	return block->query_ret;
} // }}}

void *  ipc_shmem_listen  (void *ipc){ // {{{
	uint32_t blockid;
	while(1)
		shmem_do((ipc_t *)ipc, WND_READ, &callb_read, NULL, &blockid);
} // }}}
ssize_t ipc_shmem_init    (ipc_t *ipc, config_t *config){ // {{{
	ssize_t             ret;
	int                 shmid;
	DT_INT32            shmkey;
	DT_SIZET            nitems;
	DT_SIZET            item_size = ITEM_SIZE_DEFAULT;
	DT_SIZET            shmsize;
	DT_STRING           role_str;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	hash_data_copy(ret, TYPE_SIZET,  item_size,  config, HK(item_size));
	hash_data_copy(ret, TYPE_SIZET,  nitems,     config, HK(size));      if(ret != 0) return -EINVAL;
	hash_data_copy(ret, TYPE_INT32,  shmkey,     config, HK(key));       if(ret != 0) return -EINVAL;
	hash_data_copy(ret, TYPE_STRING, role_str,   config, HK(role));      if(ret != 0) return -EINVAL;
	
	shmsize = nitems * sizeof(ipc_shmem_block) + nitems * item_size + sizeof(ipc_shmem_header); 
	
	if( (shmid = shmget(shmkey, shmsize, IPC_CREAT | 0666)) < 0)
		return -EFAULT;
	
	if( (userdata->shmaddr = shmat(shmid, NULL, 0)) == (void *)-1)
		return -EFAULT;
	
	if( (userdata->role = ipc_string_to_role(role_str)) == ROLE_INVALID)
		return -EFAULT;
	
	userdata->shmblocks = (ipc_shmem_block *)((void *)userdata->shmaddr   + sizeof(ipc_shmem_header));
	userdata->shmdata   = (void *)           ((void *)userdata->shmblocks + nitems * sizeof(ipc_shmem_block));
	userdata->item_size = item_size;
	userdata->nitems    = nitems;
	userdata->inited    = 1;
	
	if(userdata->role == ROLE_SERVER){
		shmem_init(userdata);
		
		// start threads
		if(pthread_create(&userdata->server_thr, NULL, &ipc_shmem_listen, ipc) != 0)
			return -EFAULT;
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
		
		sem_destroy(&userdata->shmaddr->sem[WND_WRITE]);
		sem_destroy(&userdata->shmaddr->sem[WND_READ]);
	}
	
error:
	shmdt(userdata->shmaddr);
	userdata->inited = 0;
	return 0;
} // }}}
ssize_t ipc_shmem_query   (ipc_t *ipc, request_t *request){ // {{{
	ssize_t             ret;
	uint32_t            blockid;
	
	ret = shmem_do(ipc, WND_WRITE,  &callb_write,  request, &blockid);
	//ret = 0x0000BEEF;
	ret = shmem_do(ipc, WND_RETURN, &callb_return, request, &blockid);
	return ret;
} // }}}

