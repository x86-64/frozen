#include <libfrozen.h>

#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <numeric/uint/uint_t.h>

#define EMODULE 50

typedef struct allocator_list_t {
	uintmax_t              item_size;
	uintmax_t              last_id;
	data_t                 storage;
	pthread_rwlock_t       rwlock;
} allocator_list_t;

static ssize_t   allocator_getnewid(allocator_list_t *fdata, uintmax_t *newid){ // {{{
	ssize_t                ret;
	
	if(__MAX(uintmax_t) / fdata->item_size <= fdata->last_id)
		return -ENOSPC;
	
	*newid = fdata->last_id++;
	
	fastcall_resize r_resize = {
		{ 3, ACTION_RESIZE },
		fdata->last_id * fdata->item_size
	};
	if( (ret = data_query(&fdata->storage, &r_resize)) < 0){
		fdata->last_id--;
		return ret;
	}
	return 0;
} // }}}
static ssize_t   allocator_removelastid(allocator_list_t *fdata){ // {{{
	ssize_t                ret;
	uintmax_t              id;
	
	id = --fdata->last_id;
	
	fastcall_resize r_resize = {
		{ 3, ACTION_RESIZE },
		id * fdata->item_size
	};
	if( (ret = data_query(&fdata->storage, &r_resize)) < 0){
		fdata->last_id++;
		return ret;
	}
	return 0;
} // }}}

static void      allocator_destroy(allocator_list_t *fdata){ // {{{
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&fdata->storage, &r_free);
	
	pthread_rwlock_destroy(&fdata->rwlock);
	free(fdata);
} // }}}
static ssize_t   allocator_new(allocator_list_t **pfdata, hash_t *config){ // {{{
	ssize_t                ret;
	data_t                *sample;
	allocator_list_t      *fdata;
	
	if( (fdata = calloc(1, sizeof(allocator_list_t))) == NULL)
		return -ENOMEM;
	
	pthread_rwlock_init(&fdata->rwlock, NULL);
	
	hash_holder_consume(ret, fdata->storage, config, HK(storage));
	if(ret != 0){
		ret = error("invalid storage");
		goto error;
	}
	
	hash_data_get(ret, TYPE_UINTT, fdata->item_size, config, HK(item_size));
	if(ret != 0){
		if( (sample = hash_data_find(config, HK(item_sample))) == NULL){
			ret = error("no item_size nor item_sample supplied");
			goto error;
		}
		
		fastcall_physicallen r_len = { { 3, ACTION_PHYSICALLEN } };
		if( data_query(sample, &r_len) != 0 ){
			ret = error("bad item_sample");
			goto error;
		}
		
		fdata->item_size = r_len.length;
	}
	if(fdata->item_size == 0){
		ret = error("bad item size");
		goto error;
	}
	
	// get last id
	fastcall_logicallen r_len = { { 3, ACTION_LOGICALLEN } };
	if( data_query(&fdata->storage, &r_len) != 0){
		ret = error("bad underlying storage");
		goto error;
	}
	
	fdata->last_id = r_len.length / fdata->item_size;
	
	*pfdata = fdata;
	return 0;

error:
	allocator_destroy(fdata);
	return ret;
} // }}}

static ssize_t data_allocator_list_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->src == NULL)
		return -EINVAL; 
		
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t            *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return allocator_new((allocator_list_t **)&data->ptr, config);
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_allocator_list_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr != NULL)
		allocator_destroy(data->ptr);
	
	return 0;
} // }}}

static ssize_t data_allocator_list_t_default(data_t *data, fastcall_header *hargs){
	ssize_t                ret;
	uintmax_t              id;
	allocator_list_t      *fdata          = (allocator_list_t *)data->ptr;
	
	switch(hargs->action){
		case ACTION_CREATE:;
			fastcall_create *r_create = (fastcall_create *)hargs;
			
			pthread_rwlock_wrlock(&fdata->rwlock);
				
				ret = allocator_getnewid(fdata, &r_create->offset);
			
			break;
		
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io *r_io = (fastcall_io *)hargs;
			
			pthread_rwlock_rdlock(&fdata->rwlock);
				
				if(r_io->offset >= fdata->last_id){
					ret = -EINVAL;
					break;
				}
				
				if(safe_mul(&r_io->offset, r_io->offset, fdata->item_size) != 0){
					ret = -EINVAL;
					break;
				}
				
				ret = data_query(&fdata->storage, r_io);
			
			break;
		
		case ACTION_DELETE:;
			//fastcall_delete *r_delete = (fastcall_delete *)hargs;
			
			pthread_rwlock_wrlock(&fdata->rwlock);
				
				// TODO move items
				//ret = allocator_removelastid(fdata);
				
				ret = -ENOSYS;
			break;
		
		case ACTION_COUNT:;
			fastcall_count *r_count = (fastcall_count *)hargs;
			
			pthread_rwlock_rdlock(&fdata->rwlock);
				
				r_count->nelements = fdata->last_id - 1;
				ret = 0;

			break;
		
		case ACTION_PUSH:;
			fastcall_push *r_push = (fastcall_push *)hargs;
			
			pthread_rwlock_wrlock(&fdata->rwlock);
				
				if( (ret = allocator_getnewid(fdata, &id)) < 0)
					break;
				
				fastcall_write r_write = {
					{ 5, ACTION_WRITE },
					id * fdata->item_size,
					r_push->buffer,
					MIN(r_push->buffer_size, fdata->item_size)
				};
				if( (ret = data_query(&fdata->storage, &r_write)) < 0)
					goto push_unwind;
				
				r_push->buffer_size = r_write.buffer_size;
				
			break;
		
		push_unwind:
			fdata->last_id--;
			break;
			
		case ACTION_POP:;
			fastcall_pop *r_pop = (fastcall_pop *)hargs;
			
			pthread_rwlock_wrlock(&fdata->rwlock);
				
				if(fdata->last_id == 0){
					ret = -EINVAL;
					break;
				}
				
				id = fdata->last_id - 1;
				
				fastcall_read r_read = {
					{ 5, ACTION_READ },
					id * fdata->item_size,
					r_pop->buffer,
					MIN(r_pop->buffer_size, fdata->item_size)
				};
				if( (ret = data_query(&fdata->storage, &r_read)) < 0)
					break;
				
				r_pop->buffer_size = r_read.buffer_size;
				
				ret = allocator_removelastid(fdata);
			
			break;

		default:
			ret = -ENOSYS;
	}
	pthread_rwlock_unlock(&fdata->rwlock);
	return ret;
}

data_proto_t allocator_list_t_proto = {
        .type                   = TYPE_ALLOCATORLISTT,
        .type_str               = "allocator_list_t",
        .api_type               = API_HANDLERS,
        .handler_default        = (f_data_func)&data_allocator_list_t_default,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_allocator_list_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_allocator_list_t_free,
	}
};

