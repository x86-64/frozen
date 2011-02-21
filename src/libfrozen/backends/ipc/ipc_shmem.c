#include <libfrozen.h>
#include <backends/ipc/ipc.h>
#include <backends/ipc/ipc_shmem.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

#define ITEM_SIZE_DEFAULT 1000
#define FAIL_SLEEP        1000

typedef struct ipc_shmem_header {
	volatile uint32_t      write_end;
	volatile uint32_t      write_start;
	volatile uint32_t      read_end;
	volatile uint32_t      read_start;
} ipc_shmem_header;

typedef struct ipc_shmem_block {
	volatile uint32_t      write_done;
	volatile uint32_t      read_done;
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

typedef ssize_t (*func_read_callb) (ipc_t *ipc, void *data);
	
static inline uint32_t shmem_next(ipc_shmem_userdata *userdata, uint32_t curr){ // {{{
	uint32_t next = curr + 1;
	return (next >= userdata->nitems) ? 0 : next;
} // }}}

static ssize_t shmem_init(ipc_shmem_userdata *userdata){ // {{{
	userdata->shmaddr->write_end   = 1;
	userdata->shmaddr->write_start = 1;
	userdata->shmaddr->read_end    = 0;
	userdata->shmaddr->read_start  = 0;
	return 0;
} // }}}
static ssize_t shmem_write(ipc_t *ipc, buffer_t *buffer){ // {{{
	size_t              buffer_size;
	uint32_t            blockid, blockid_next;
	void               *block_data;
	ipc_shmem_block    *block;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	// check data
	buffer_size = buffer_get_size(buffer);
	if(buffer_size > userdata->item_size)
		return -EINVAL;
	
	// reserve block
	for(;;){
		blockid = userdata->shmaddr->write_start;
		blockid_next = shmem_next(userdata, blockid);
		if(blockid_next == userdata->shmaddr->read_end){
			usleep(FAIL_SLEEP);
			continue;
		}
		
		if(__sync_bool_compare_and_swap(&userdata->shmaddr->write_start, blockid, blockid_next))
			break;
	}
	block      = &userdata->shmblocks[blockid];
	block_data = userdata->shmdata + (blockid * userdata->item_size);
	
	// read data from buffer to memory
	buffer_read(buffer, 0, block_data, buffer_size);
	
	// send block
	block->read_done  = 0;
	block->write_done = 1;
	
	for(;;){
		blockid = userdata->shmaddr->write_end;
		block   = &userdata->shmblocks[blockid];
		
		if(!__sync_bool_compare_and_swap(&block->write_done, 1, 0))
			break;
		
		__sync_bool_compare_and_swap(&userdata->shmaddr->write_end, blockid, shmem_next(userdata, blockid));
	}
	return 0;
} // }}}
static ssize_t shmem_read(ipc_t *ipc, func_read_callb callb){ // {{{
	ssize_t             ret;
	uint32_t            blockid, blockid_next;
	void               *block_data;
	ipc_shmem_block    *block;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	// reserve block
	while(1){
		blockid      = userdata->shmaddr->read_start;
		blockid_next = shmem_next(userdata, blockid);
		if(blockid_next == userdata->shmaddr->write_end){
			usleep(FAIL_SLEEP);
			continue;
		}
		
		if(__sync_bool_compare_and_swap(&userdata->shmaddr->read_start, blockid, blockid_next))
			break;
	}
	block      = &userdata->shmblocks[blockid];
	block_data = userdata->shmdata + (blockid * userdata->item_size);
	
	// call callback
	ret = callb(ipc, block_data);
	
	// send block
	block->write_done = 0;
	block->read_done  = 1;
	
	while(1){
		blockid = userdata->shmaddr->read_end;
		block   = &userdata->shmblocks[blockid];
		
		if(!__sync_bool_compare_and_swap(&block->read_done, 1, 0))
			break;
		
		__sync_bool_compare_and_swap(&userdata->shmaddr->read_end, blockid, shmem_next(userdata, blockid));
	}
	return ret;
} // }}}

ssize_t shmem_recvsig(ipc_t *ipc, void *data){ // {{{
	ssize_t             ret;
	buffer_t            buffer;
	request_t          *request;
	ipc_shmem_userdata *userdata = (ipc_shmem_userdata *)ipc->userdata;
	
	buffer_init_from_bare(&buffer, data, userdata->item_size);
	
	if(hash_from_buffer(&request, &buffer) < 0)
		return -EFAULT;
	
	ret = chain_next_query(ipc->chain, request);
	
	buffer_destroy(&buffer);
	
	return ret;
} // }}}
void *  shmem_recv_thread(void *ipc){ // {{{
	while(1)
		shmem_read((ipc_t *)ipc, &shmem_recvsig);
} // }}}

ssize_t ipc_shmem_init  (ipc_t *ipc, config_t *config){ // {{{
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
		if(pthread_create(&userdata->server_thr, NULL, &shmem_recv_thread, ipc) != 0)
			return -EFAULT;
	}
	return 0;
} // }}}
ssize_t ipc_shmem_destroy(ipc_t *ipc){ // {{{
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
ssize_t ipc_shmem_query (ipc_t *ipc, buffer_t *buffer){ // {{{
	return shmem_write(ipc, buffer);
} // }}}

