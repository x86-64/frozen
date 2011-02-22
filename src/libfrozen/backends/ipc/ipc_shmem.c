#include <libfrozen.h>
#include <backends/ipc/ipc.h>
#include <backends/ipc/ipc_shmem.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#define ITEM_SIZE_DEFAULT 1000
#define FAIL_SLEEP        1

typedef enum window_type {
	WND_WRITE  = 0,
	WND_READ   = 1,
	WND_RETURN = 2
} window_type;

typedef struct window_t {
	volatile uint32_t      end;
	volatile uint32_t      start;
} window_t;

typedef struct ipc_shmem_header {
	window_t               windows[3];
} ipc_shmem_header;

typedef struct ipc_shmem_block {
	volatile ssize_t       query_ret;
	volatile uint8_t       done[3];
} ipc_shmem_block;

typedef struct ipc_shmem_userdata {
	int               inited;
	ipc_role          role;
	
	size_t            item_size;
	size_t            nitems;
	
	pthread_t         server_thr;
	
	ipc_shmem_header *shmaddr;
	ipc_shmem_block  *shmblocks;
	void             *shmdata;
} ipc_shmem_userdata;

typedef ssize_t (*func_callb) (ipc_t *ipc, ipc_shmem_block *block, void *data, void *arg);
	
static inline uint32_t shmem_next(ipc_shmem_userdata *userdata, uint32_t curr){ // {{{
	uint32_t next = curr + 1;
	return (next >= userdata->nitems) ? 0 : next;
} // }}}
static inline window_type window_depends(window_type type){ // {{{
	switch(type){
		case WND_WRITE:  return WND_WRITE;
		case WND_READ:   return WND_WRITE;
		case WND_RETURN: return WND_READ;
	}
	return -1;
} // }}}

static ssize_t shmem_init(ipc_shmem_userdata *userdata){ // {{{
	/*
	userdata->shmaddr->windows[WND_WRITE].end    = 1;
	userdata->shmaddr->windows[WND_WRITE].start  = 1;
	userdata->shmaddr->windows[WND_READ].end     = 0;
	userdata->shmaddr->windows[WND_READ].start   = 0;
	userdata->shmaddr->windows[WND_RETURN].end   = 1;
	userdata->shmaddr->windows[WND_RETURN].start = 1;*/
	return 0;
} // }}}
static ssize_t shmem_do(ipc_t *ipc, window_type window, func_callb callb, void *arg, uint32_t *p_blockid){ // {{{
	ssize_t             ret;
	uint32_t            blockid, blockid_next;
	window_t           *window_curr, *window_deps;
	void               *block_data;
	ipc_shmem_block    *block;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	window_curr = &userdata->shmaddr->windows[window];
	window_deps = &userdata->shmaddr->windows[window_depends(window)];
	
	// reserve block
	while(1){
		blockid      = window_curr->start;
		blockid_next = shmem_next(userdata, blockid);
		
		if(window == WND_WRITE){
			if(blockid_next == window_deps->end) goto sleep;
		}else{
			if(blockid      == window_deps->end) goto sleep;
		}
		
		if(__sync_bool_compare_and_swap(&window_curr->start, blockid, blockid_next))
			break;
		
		continue;
sleep:
		usleep(FAIL_SLEEP);
	}
	
	if(p_blockid)
		*p_blockid = blockid;
	block      = &userdata->shmblocks[blockid];
	block_data =  userdata->shmdata + (blockid * userdata->item_size);
	
	// call callback
	ret = callb(ipc, block, block_data, arg);
	
	block->done[WND_WRITE]  = (WND_WRITE  == window) ? 1 : 0;
	block->done[WND_READ]   = (WND_READ   == window) ? 1 : 0;
	block->done[WND_RETURN] = (WND_RETURN == window) ? 1 : 0;
	
	while(1){
		blockid      = window_curr->end;
		blockid_next = shmem_next(userdata, blockid);
		block        = &userdata->shmblocks[blockid];
		
		if(!__sync_bool_compare_and_swap(&block->done[window], 1, 0))
			break;
		
		__sync_bool_compare_and_swap(&window_curr->end, blockid, blockid_next);
	}
	return ret;
} // }}}

ssize_t callb_write(ipc_t *ipc, ipc_shmem_block *block, void *data, void *p_buffer){ // {{{
	buffer_read((buffer_t *)p_buffer, 0, data, buffer_get_size((buffer_t *)p_buffer));
	return 0;
} // }}}
ssize_t callb_read(ipc_t *ipc, ipc_shmem_block *block, void *data, void *null){ // {{{
	ssize_t             ret;
	buffer_t            buffer;
	request_t          *request;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	buffer_init_from_bare(&buffer, data, userdata->item_size);
	
	if(hash_from_buffer(&request, &buffer) < 0){
		ret = -EFAULT;
		goto exit;
	}
	
	ret = chain_next_query(ipc->chain, request);
exit:
	buffer_destroy(&buffer);
	block->query_ret = ret;
	return ret;
} // }}}
ssize_t callb_return(ipc_t *ipc, ipc_shmem_block *block, void *data, void *p_buffer){ // {{{
	off_t zzz = chunk_get_size(((buffer_t *)p_buffer)->head); // HEEEEELLLLL
	buffer_write((buffer_t *)p_buffer, zzz, data + zzz, buffer_get_size((buffer_t *)p_buffer) - zzz);
	
	return block->query_ret;
} // }}}

void *  ipc_shmem_listen  (void *ipc){ // {{{
	while(1)
		shmem_do((ipc_t *)ipc, WND_READ, &callb_read, NULL, NULL);
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
	
	hash_copy_data(ret, TYPE_SIZET,  item_size,  config, HK(item_size));
	hash_copy_data(ret, TYPE_SIZET,  nitems,     config, HK(size));      if(ret != 0) return -EINVAL;
	hash_copy_data(ret, TYPE_INT32,  shmkey,     config, HK(key));       if(ret != 0) return -EINVAL;
	hash_copy_data(ret, TYPE_STRING, role_str,   config, HK(role));      if(ret != 0) return -EINVAL;
	
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
	}
	
error:
	shmdt(userdata->shmaddr);
	userdata->inited = 0;
	return 0;
} // }}}
ssize_t ipc_shmem_query   (ipc_t *ipc, buffer_t *buffer){ // {{{
	ssize_t             ret;
	uint32_t            blockid;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	if(buffer_get_size(buffer) > userdata->item_size)
		return -ENOMEM;
	
	ret = shmem_do(ipc, WND_WRITE,  &callb_write,  buffer, &blockid);
	ret = shmem_do(ipc, WND_RETURN, &callb_return, buffer, &blockid);
	return ret;
} // }}}

