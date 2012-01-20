#include <libfrozen.h>

#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <numeric/uint/uint_t.h>

#define EMODULE 49

typedef struct allocator_fixed_t {
	uintmax_t              item_size;
	uintmax_t              last_id;
	data_t                 storage;
	data_t                 removed_items;
	pthread_rwlock_t       last_id_mtx;
} allocator_fixed_t;

static ssize_t   allocator_getnewid(allocator_fixed_t *fdata, uintmax_t *newid){ // {{{
	register ssize_t       ret               = 0;
	
	pthread_rwlock_wrlock(&fdata->last_id_mtx);
		
		if(__MAX(uintmax_t) / fdata->item_size <= fdata->last_id){
			ret = -ENOSPC;
		}else{
			*newid = fdata->last_id++;
		}
		
	pthread_rwlock_unlock(&fdata->last_id_mtx);
	return ret;
} // }}}
static uintmax_t allocator_getlastid(allocator_fixed_t *fdata){ // {{{
	uintmax_t              last_id;

	pthread_rwlock_rdlock(&fdata->last_id_mtx);
		last_id = fdata->last_id;
	pthread_rwlock_unlock(&fdata->last_id_mtx);
	return last_id;
} // }}}

static void allocator_destroy(allocator_fixed_t *fdata){ // {{{
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&fdata->storage, &r_free);
	
	pthread_rwlock_destroy(&fdata->last_id_mtx);
	free(fdata);
} // }}}
static ssize_t allocator_new(allocator_fixed_t **pfdata, hash_t *config){ // {{{
	ssize_t                ret;
	data_t                *sample;
	allocator_fixed_t     *fdata;
	
	if((fdata = calloc(1, sizeof(allocator_fixed_t))) == NULL)
		return error("calloc failed");
	
	pthread_rwlock_init(&fdata->last_id_mtx, NULL);
	
	// get removed items tracker
	hash_holder_consume(ret, fdata->removed_items, config, HK(removed_items));
	hash_holder_consume(ret, fdata->storage,       config, HK(storage));
	if(ret != 0){
		ret = error("invalid storage supplied");
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

static ssize_t data_allocator_fixed_t_convert_from(data_t *data, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	
	if(fargs->src == NULL)
		return -EINVAL; 
		
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t            *config;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return allocator_new((allocator_fixed_t **)&data->ptr, config);
			
		default:
			break;
	};
	return -ENOSYS;
} // }}}
static ssize_t data_allocator_fixed_t_free(data_t *data, fastcall_free *fargs){ // {{{
	if(data->ptr != NULL)
		allocator_destroy(data->ptr);
	
	return 0;
} // }}}

static ssize_t data_allocator_fixed_t_default(data_t *data, fastcall_header *hargs){ // {{{
	allocator_fixed_t     *fdata          = (allocator_fixed_t *)data->ptr;
	
	switch(hargs->action){
		case ACTION_CREATE:;
			fastcall_create *r_create = (fastcall_create *)hargs;
			
			if(fdata->removed_items.type){
				fastcall_pop r_pop = {
					{ 4, ACTION_POP },
					&r_create->offset,
					sizeof(r_create->offset)
				};
				if(data_query(&fdata->removed_items, &r_pop) == 0)
					return 0;
			}
			
			if(allocator_getnewid(fdata, &r_create->offset) != 0)
				return -EFAULT;
			
			return 0;
			
		case ACTION_DELETE:;
			fastcall_delete *r_delete = (fastcall_delete *)hargs;
			
			if(fdata->removed_items.type){
				fastcall_push r_push = {
					{ 4, ACTION_PUSH },
					&r_delete->offset,
					sizeof(r_delete->offset)
				};
				if(data_query(&fdata->removed_items, &r_push) != 0)
					return -EFAULT;
			}
			return 0;
		
		case ACTION_COUNT:;
			uintmax_t       in_removed = 0;
			fastcall_count *r_count    = (fastcall_count *)hargs;
			
			if(fdata->removed_items.type){
				fastcall_count r_count = { { 3, ACTION_COUNT } };
				if(data_query(&fdata->removed_items, &r_count) == 0){
					in_removed = r_count.nelements;
				}
			}
			
			r_count->nelements = allocator_getlastid(fdata) - in_removed - 1;
			return 0;
		
		case ACTION_READ:
		case ACTION_WRITE:;
			fastcall_io *r_io = (fastcall_io *)hargs;
			
			if(r_io->offset >= allocator_getlastid(fdata))
				return -EINVAL;
			
			if(safe_mul(&r_io->offset, r_io->offset, fdata->item_size) != 0)
				return -EINVAL;
			
			return data_query(&fdata->storage, r_io);
		
		default:
			break;
	}
	return -ENOSYS;
} // }}}

data_proto_t allocator_fixed_t_proto = {
        .type                   = TYPE_ALLOCATORFIXEDT,
        .type_str               = "allocator_fixed_t",
        .api_type               = API_HANDLERS,
        .handler_default        = (f_data_func)&data_allocator_fixed_t_default,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_allocator_fixed_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_allocator_fixed_t_free,
	}
};

