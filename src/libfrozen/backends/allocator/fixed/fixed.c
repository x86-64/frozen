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
 *              item_size               = (uint_t)'10',       # sepcify size directly             OR
 *              item_sample             = (sometype_t)'',     # sample item for size estimation
 *
 * }
 * @endcode
 */

#define EMODULE 49

typedef struct allocator_userdata {
	uintmax_t              item_size;
	uintmax_t              last_id;
	pthread_rwlock_t       last_id_mtx;
} allocator_userdata;

static ssize_t   allocator_getnewid(allocator_userdata *userdata, uintmax_t *newid){ // {{{
	register ssize_t       ret               = 0;
	
	pthread_rwlock_wrlock(&userdata->last_id_mtx);
		if(userdata->last_id != ~(uintmax_t)0){
			*newid = userdata->last_id++;
		}else{
			ret = -EINVAL;
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
	}
	
	
	return 0;
} // }}}

static ssize_t allocator_fast_handler(backend_t *backend, fastcall_header *hargs){
	allocator_userdata    *userdata          = (allocator_userdata *)backend->userdata;
	
	switch(hargs->action){
		case ACTION_CREATE:;
			fastcall_create *r_create = (fastcall_create *)hargs;
			
			// ask removed items list
			
			if(allocator_getnewid(userdata, &r_create->offset) != 0)
				return -EFAULT;
			
			return 0;
			
		case ACTION_DELETE:
			// add to removed items list
			break;
		
		case ACTION_COUNT:;
			fastcall_count *r_count = (fastcall_count *)hargs;
			// get removed items list length
			
			r_count->nelements = allocator_getlastid(userdata) - 1; // - del_items_count;
			return 0;
		
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io *r_io = (fastcall_io *)hargs;
			
			//if(r_io->offset >= allocator_getlastid(userdata))
			//	return -EINVAL;
			
			if(safe_mul(&r_io->offset, r_io->offset, userdata->item_size) != 0)
				return -EINVAL;
			
			return backend_fast_pass(backend, r_io);
		
		default:
			break;
	}
	return -ENOSYS;
}

/*static ssize_t allocator_handler(backend_t *backend, request_t *request){
	ssize_t                ret               = 0;
	uintmax_t              action;
	
	hash_data_copy(ret, TYPE_UINTT, action, request, HK(action));
	if(ret != 0)
		return -ENOSYS;
	
	return ret;
}*/

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

