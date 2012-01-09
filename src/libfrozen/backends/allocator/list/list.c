#include <libfrozen.h>

/**
 * @ingroup backend
 * @addtogroup mod_backend_list allocator/list
 */
/**
 * @ingroup mod_backend_list
 * @page page_list_info Description
 *
 * This backend allocate and delete items from storage* backends or from clean datatypes. Data kept in form of list.
 * Items can change their id's after delete actions.
 *
 * This backend support PUSH/POP api set.
 */
/**
 * @ingroup mod_backend_list
 * @page page_list_config Configuration
 * 
 * Accepted configuration:
 * @code
 * {
 *              class                   = "allocator/list",
 *              item_size               = (uint_t)'10',       # specify size directly             OR
 *              item_sample             = (sometype_t)'',     # sample item for size estimation
 *
 * }
 * @endcode
 */

#define EMODULE 50

typedef struct allocator_userdata {
	uintmax_t              item_size;
	uintmax_t              last_id;
	pthread_rwlock_t       rwlock;
} allocator_userdata;

static ssize_t   allocator_getnewid(backend_t *backend, uintmax_t *newid){ // {{{
	ssize_t                ret;
	allocator_userdata    *userdata          = (allocator_userdata *)backend->userdata;
	
	if(__MAX(uintmax_t) / userdata->item_size <= userdata->last_id)
		return -ENOSPC;
	
	*newid = userdata->last_id++;
	
	fastcall_resize r_resize = {
		{ 3, ACTION_RESIZE },
		userdata->last_id * userdata->item_size
	};
	if( (ret = backend_fast_pass(backend, &r_resize)) < 0){
		userdata->last_id--;
		return ret;
	}
	return 0;
} // }}}
static ssize_t   allocator_removelastid(backend_t *backend){ // {{{
	ssize_t                ret;
	uintmax_t              id;
	allocator_userdata    *userdata          = (allocator_userdata *)backend->userdata;
	
	id = --userdata->last_id;
	
	fastcall_resize r_resize = {
		{ 3, ACTION_RESIZE },
		id * userdata->item_size
	};
	if( (ret = backend_fast_pass(backend, &r_resize)) < 0){
		userdata->last_id++;
		return ret;
	}
	return 0;
} // }}}

static int allocator_init(backend_t *backend){ // {{{
	allocator_userdata    *userdata;
	
	if((userdata = backend->userdata = calloc(1, sizeof(allocator_userdata))) == NULL)
		return error("calloc failed");
	
	pthread_rwlock_init(&userdata->rwlock, NULL);
	return 0;
} // }}}
static int allocator_destroy(backend_t *backend){ // {{{
	allocator_userdata    *userdata          = (allocator_userdata *)backend->userdata;
	
	pthread_rwlock_destroy(&userdata->rwlock);
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
	ssize_t                ret;
	uintmax_t              id;
	allocator_userdata    *userdata          = (allocator_userdata *)backend->userdata;
	
	switch(hargs->action){
		case ACTION_CREATE:;
			fastcall_create *r_create = (fastcall_create *)hargs;
			
			pthread_rwlock_wrlock(&userdata->rwlock);
				
				ret = allocator_getnewid(backend, &r_create->offset);
			
			break;
		
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io *r_io = (fastcall_io *)hargs;
			
			pthread_rwlock_rdlock(&userdata->rwlock);
				
				if(r_io->offset >= userdata->last_id){
					ret = -EINVAL;
					break;
				}
				
				if(safe_mul(&r_io->offset, r_io->offset, userdata->item_size) != 0){
					ret = -EINVAL;
					break;
				}
				
				ret = backend_fast_pass(backend, r_io);
			
			break;
		
		case ACTION_DELETE:;
			//fastcall_delete *r_delete = (fastcall_delete *)hargs;
			
			pthread_rwlock_wrlock(&userdata->rwlock);
				
				// TODO move items
				//ret = allocator_removelastid(backend);
				
				ret = -ENOSYS;
			break;
		
		case ACTION_COUNT:;
			fastcall_count *r_count = (fastcall_count *)hargs;
			
			pthread_rwlock_rdlock(&userdata->rwlock);
				
				r_count->nelements = userdata->last_id - 1;
				ret = 0;

			break;
		
		case ACTION_PUSH:;
			fastcall_push *r_push = (fastcall_push *)hargs;
			
			pthread_rwlock_wrlock(&userdata->rwlock);
				
				if( (ret = allocator_getnewid(backend, &id)) < 0)
					break;
				
				fastcall_write r_write = {
					{ 5, ACTION_WRITE },
					id * userdata->item_size,
					r_push->buffer,
					MIN(r_push->buffer_size, userdata->item_size)
				};
				if( (ret = backend_fast_pass(backend, &r_write)) < 0)
					goto push_unwind;
				
				r_push->buffer_size = r_write.buffer_size;
				
			break;
		
		push_unwind:
			userdata->last_id--;
			break;
			
		case ACTION_POP:;
			fastcall_pop *r_pop = (fastcall_pop *)hargs;
			
			pthread_rwlock_wrlock(&userdata->rwlock);
				
				if(userdata->last_id == 0){
					ret = -EINVAL;
					break;
				}
				
				id = userdata->last_id - 1;
				
				fastcall_read r_read = {
					{ 5, ACTION_READ },
					id * userdata->item_size,
					r_pop->buffer,
					MIN(r_pop->buffer_size, userdata->item_size)
				};
				if( (ret = backend_fast_pass(backend, &r_read)) < 0)
					break;
				
				r_pop->buffer_size = r_read.buffer_size;
				
				ret = allocator_removelastid(backend);
			
			break;

		default:
			return -ENOSYS;
	}
	pthread_rwlock_unlock(&userdata->rwlock);
	return ret;
}

/*static ssize_t allocator_handler(backend_t *backend, request_t *request){
	ssize_t                ret               = 0;
	uintmax_t              action;
	
	hash_data_copy(ret, TYPE_UINTT, action, request, HK(action));
	if(ret != 0)
		return -ENOSYS;
	
	return ret;
}*/

backend_t list_proto = {
	.class          = "allocator/list",
	.supported_api  = API_FAST,
	.func_init      = &allocator_init,
	.func_configure = &allocator_configure,
	.func_destroy   = &allocator_destroy,
	.backend_type_fast = {
		.func_handler = (f_fast_func)&allocator_fast_handler,
	}
};

