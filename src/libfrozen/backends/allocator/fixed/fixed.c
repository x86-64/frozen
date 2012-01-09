#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_fixed allocator/fixed
 */
/**
 * @ingroup mod_backend_fixed
 * @page page_fixed_info Description
 *
 * This backend allocate and delete items from storage* backends or from clean datatypes.
 */
/**
 * @ingroup mod_backend_fixed
 * @page page_fixed_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "allocator/fixed",
 *              item_size               = (uint_t)'10',                            # specify size directly             OR
 *              item_sample             = (sometype_t)'',                          # sample item for size estimation
 *              removed_items           = {                                        # removed items tracker, if not supplied - no tracking performed
 *                   { class = "allocator/list", item_sample = (uint_t)"0" },      # - this or similar configuration expected
 *                   { <memory or file backend to store info>              } 
 *              }
 *
 * }
 * @endcode
 */
/**
 * @ingroup mod_backend_fixed
 * @page page_fixed_reqs Removed items tracker requirements
 *
 * @li Thread safety. Required.
 * @li Support ACTION_PUSH and ACTION_POP functions. Required.
 * @li Support ACTION_COUNT. Optional, for accurate measurement of elements count.
 */

#define EMODULE 49

typedef struct allocator_userdata {
	backend_t             *removed_items;
	uintmax_t              item_size;
	uintmax_t              last_id;
	pthread_rwlock_t       last_id_mtx;
} allocator_userdata;

static ssize_t   allocator_getnewid(allocator_userdata *userdata, uintmax_t *newid){ // {{{
	register ssize_t       ret               = 0;
	
	pthread_rwlock_wrlock(&userdata->last_id_mtx);
		
		if(__MAX(uintmax_t) / userdata->item_size <= userdata->last_id){
			ret = -ENOSPC;
		}else{
			*newid = userdata->last_id++;
		}
		
	pthread_rwlock_unlock(&userdata->last_id_mtx);
	return ret;
} // }}}
static uintmax_t allocator_getlastid(allocator_userdata *userdata){ // {{{
	uintmax_t              last_id;

	pthread_rwlock_rdlock(&userdata->last_id_mtx);
		last_id = userdata->last_id;
	pthread_rwlock_unlock(&userdata->last_id_mtx);
	return last_id;
} // }}}

static int allocator_init(backend_t *backend){ // {{{
	allocator_userdata    *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(allocator_userdata))) == NULL)
		return error("calloc failed");
	
	pthread_rwlock_init(&userdata->last_id_mtx, NULL);
	return 0;
} // }}}
static int allocator_destroy(backend_t *backend){ // {{{
	allocator_userdata    *userdata          = (allocator_userdata *)backend->userdata;
	
	pthread_rwlock_destroy(&userdata->last_id_mtx);
	free(userdata);
	return 0;
} // }}}
static int allocator_configure(backend_t *backend, hash_t *config){ // {{{
	ssize_t                ret;
	data_t                *sample;
	allocator_userdata    *userdata          = (allocator_userdata *)backend->userdata;
	
	// get item size
	if(userdata->item_size == 0){
		hash_data_copy(ret, TYPE_UINTT,    userdata->item_size, config, HK(item_size));
		if(ret != 0){
			if( (sample = hash_data_find(config, HK(item_sample))) == NULL)
				return error("no item_size nor item_sample supplied");
			
			fastcall_physicallen r_len = { { 3, ACTION_PHYSICALLEN } };
			if( data_query(sample, &r_len) != 0 )
				return error("bad item_sample");
			
			userdata->item_size = r_len.length;
		}
		if(userdata->item_size == 0)
			return error("bad item size");
		
		// get last id
		fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
		if( backend_fast_pass(backend, &r_len) != 0)
			return error("bad underlying storage");
		
		userdata->last_id = r_len.length / userdata->item_size;
		
		// get removed items tracker
		hash_data_copy(ret, TYPE_BACKENDT,  userdata->removed_items, config, HK(removed_items));
	}
	
	return 0;
} // }}}

static ssize_t allocator_fast_handler(backend_t *backend, fastcall_header *hargs){ // {{{
	allocator_userdata    *userdata          = (allocator_userdata *)backend->userdata;
	
	switch(hargs->action){
		case ACTION_CREATE:;
			fastcall_create *r_create = (fastcall_create *)hargs;
			
			if(userdata->removed_items){
				fastcall_pop r_pop = {
					{ 4, ACTION_POP },
					&r_create->offset,
					sizeof(r_create->offset)
				};
				if(backend_fast_query(userdata->removed_items, &r_pop) == 0)
					return 0;
			}
			
			if(allocator_getnewid(userdata, &r_create->offset) != 0)
				return -EFAULT;
			
			return 0;
			
		case ACTION_DELETE:;
			fastcall_delete *r_delete = (fastcall_delete *)hargs;
			
			if(userdata->removed_items){
				fastcall_push r_push = {
					{ 4, ACTION_PUSH },
					&r_delete->offset,
					sizeof(r_delete->offset)
				};
				if(backend_fast_query(userdata->removed_items, &r_push) != 0)
					return -EFAULT;
			}
			return 0;
		
		case ACTION_COUNT:;
			uintmax_t       in_removed = 0;
			fastcall_count *r_count    = (fastcall_count *)hargs;
			
			if(userdata->removed_items){
				fastcall_count r_count = { { 3, ACTION_COUNT } };
				if(backend_fast_query(userdata->removed_items, &r_count) == 0){
					in_removed = r_count.nelements;
				}
			}
			
			r_count->nelements = allocator_getlastid(userdata) - in_removed - 1;
			return 0;
		
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io *r_io = (fastcall_io *)hargs;
			
			if(r_io->offset >= allocator_getlastid(userdata))
				return -EINVAL;
			
			if(safe_mul(&r_io->offset, r_io->offset, userdata->item_size) != 0)
				return -EINVAL;
			
			return backend_fast_pass(backend, r_io);
		
		default:
			break;
	}
	return -ENOSYS;
} // }}}

backend_t fixed_proto = {
	.class          = "allocator/fixed",
	.supported_api  = API_FAST,
	.func_init      = &allocator_init,
	.func_configure = &allocator_configure,
	.func_destroy   = &allocator_destroy,
	.backend_type_fast = {
		.func_handler = (f_fast_func)&allocator_fast_handler,
	}
};

