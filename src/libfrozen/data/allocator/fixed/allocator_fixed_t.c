#include <libfrozen.h>

#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <numeric/uint/uint_t.h>

#define ERRORS_MODULE_ID 49
#define ERRORS_MODULE_NAME "allocator/fixed"

typedef struct allocator_fixed_t {
	uintmax_t              item_size;
	uintmax_t              last_id;
	data_t                 storage;
	data_t                 removed_items;
	pthread_rwlock_t       rwlock;
} allocator_fixed_t;

static ssize_t allocator_getnewid(allocator_fixed_t *fdata, uintmax_t *newid){ // {{{
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
/*static ssize_t allocator_removelastid(allocator_fixed_t *fdata){ // {{{
	ssize_t                ret;
	uintmax_t              id;
	
	if(fdata->last_id == 0)
		return -EINVAL;
	
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
} // }}}*/

static void    allocator_destroy(allocator_fixed_t *fdata){ // {{{
	fastcall_free r_free = { { 2, ACTION_FREE } };
	data_query(&fdata->storage, &r_free);
	
	pthread_rwlock_destroy(&fdata->rwlock);
	free(fdata);
} // }}}
static ssize_t allocator_new(allocator_fixed_t **pfdata, hash_t *config){ // {{{
	ssize_t                ret;
	data_t                *sample;
	allocator_fixed_t     *fdata;
	
	if((fdata = calloc(1, sizeof(allocator_fixed_t))) == NULL)
		return error("calloc failed");
	
	pthread_rwlock_init(&fdata->rwlock, NULL);
	
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
		
		fastcall_length r_len = { { 4, ACTION_LENGTH }, 0, FORMAT(packed) };
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
	fastcall_length r_len = { { 4, ACTION_LENGTH }, 0, FORMAT(native) };
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
static ssize_t data_allocator_fixed_t_io(data_t *data, fastcall_io *fargs){ // {{{
	ssize_t                ret;
	allocator_fixed_t     *fdata          = (allocator_fixed_t *)data->ptr;
	
	pthread_rwlock_rdlock(&fdata->rwlock);
	
		if(fargs->offset >= fdata->last_id){
			ret = -EINVAL;
			goto exit;
		}
		
		if(safe_mul(&fargs->offset, fargs->offset, fdata->item_size) != 0){
			ret = -EINVAL;
			goto exit;
		}
		
		ret = data_query(&fdata->storage, fargs);

exit:
	pthread_rwlock_unlock(&fdata->rwlock);
	return ret;
} // }}}
static ssize_t data_allocator_fixed_t_create(data_t *data, fastcall_create *fargs){ // {{{
	ssize_t                ret;
	allocator_fixed_t     *fdata          = (allocator_fixed_t *)data->ptr;
	
	pthread_rwlock_wrlock(&fdata->rwlock);
		
		if(fdata->removed_items.type){
			data_t       d_offset = DATA_PTR_UINTT(&fargs->offset);
			data_t       d_copy;
			
			fastcall_pop r_pop    = {
				{ 3, ACTION_POP },
				&d_copy
			};
			if( (ret = data_query(&fdata->removed_items, &r_pop)) == 0)
				goto exit;
			
			fastcall_transfer r_transfer = { { 3, ACTION_TRANSFER }, &d_offset };
			ret = data_query(&d_copy, &r_transfer);
			goto exit;
		}
		
		ret = allocator_getnewid(fdata, &fargs->offset);
exit:
	pthread_rwlock_unlock(&fdata->rwlock);
	return ret;
} // }}}
static ssize_t data_allocator_fixed_t_delete(data_t *data, fastcall_delete *fargs){ // {{{
	ssize_t                ret;
	allocator_fixed_t     *fdata          = (allocator_fixed_t *)data->ptr;
	
	pthread_rwlock_wrlock(&fdata->rwlock);
		
		if(fdata->removed_items.type){
			data_t        d_offset = DATA_PTR_UINTT(&fargs->offset);
			data_t        d_copy;
			
			fastcall_copy r_copy = { { 3, ACTION_COPY }, &d_copy };
			if( (ret = data_query(&d_offset, &r_copy)) < 0)
				goto exit;
			
			fastcall_push r_push   = {
				{ 3, ACTION_PUSH },
				&d_copy
			};
			if(data_query(&fdata->removed_items, &r_push) != 0){
				ret = -EFAULT;
				goto exit;
			}
		}
		// TODO move items
		//ret = allocator_removelastid(fdata);
	
exit:
	pthread_rwlock_unlock(&fdata->rwlock);
	return ret;
} // }}}
static ssize_t data_allocator_fixed_t_count(data_t *data, fastcall_count *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              in_removed     = 0;
	allocator_fixed_t     *fdata          = (allocator_fixed_t *)data->ptr;
	
	pthread_rwlock_rdlock(&fdata->rwlock);
	
		if(fdata->removed_items.type){
			fastcall_count fargs = { { 3, ACTION_COUNT } };
			if(data_query(&fdata->removed_items, &fargs) == 0){
				in_removed = fargs.nelements;
			}
		}
	
		fargs->nelements = fdata->last_id - in_removed - 1;
		ret = 0;
	
	pthread_rwlock_unlock(&fdata->rwlock);
	return ret;
} // }}}
/*static ssize_t data_allocator_fixed_t_push(data_t *data, fastcall_push *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              id;
	allocator_fixed_t     *fdata          = (allocator_fixed_t *)data->ptr;
	
	pthread_rwlock_wrlock(&fdata->rwlock);
		
		if( (ret = allocator_getnewid(fdata, &id)) < 0)
			goto exit;
		
		fastcall_write r_write = {
			{ 5, ACTION_WRITE },
			id * fdata->item_size,
			fargs->buffer,
			MIN(fargs->buffer_size, fdata->item_size)
		};
		if( (ret = data_query(&fdata->storage, &r_write)) < 0)
			goto push_unwind;
		
		fargs->buffer_size = r_write.buffer_size;
	
exit:
	pthread_rwlock_unlock(&fdata->rwlock);
	return ret;

push_unwind:
	fdata->last_id--;
	goto exit;
} // }}}
static ssize_t data_allocator_fixed_t_pop(data_t *data, fastcall_pop *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              id;
	allocator_fixed_t     *fdata          = (allocator_fixed_t *)data->ptr;
	
	pthread_rwlock_wrlock(&fdata->rwlock);
		
		if(fdata->last_id == 0){
			ret = -EINVAL;
			goto exit;
		}
		
		id = fdata->last_id - 1;
		
		fastcall_read r_read = {
			{ 5, ACTION_READ },
			id * fdata->item_size,
			fargs->buffer,
			MIN(fargs->buffer_size, fdata->item_size)
		};
		if( (ret = data_query(&fdata->storage, &r_read)) < 0)
			goto exit;
		
		fargs->buffer_size = r_read.buffer_size;
		
		ret = allocator_removelastid(fdata);

exit:
	pthread_rwlock_unlock(&fdata->rwlock);
	return ret;
} // }}}*/

data_proto_t allocator_fixed_t_proto = {
        .type                   = TYPE_ALLOCATORFIXEDT,
        .type_str               = "allocator_fixed_t",
        .api_type               = API_HANDLERS,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_allocator_fixed_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_allocator_fixed_t_free,
		[ACTION_READ]         = (f_data_func)&data_allocator_fixed_t_io,
		[ACTION_WRITE]        = (f_data_func)&data_allocator_fixed_t_io,
		[ACTION_CREATE]       = (f_data_func)&data_allocator_fixed_t_create,
		[ACTION_DELETE]       = (f_data_func)&data_allocator_fixed_t_delete,
		[ACTION_COUNT]        = (f_data_func)&data_allocator_fixed_t_count,
		//[ACTION_PUSH]         = (f_data_func)&data_allocator_fixed_t_push,
		//[ACTION_POP]          = (f_data_func)&data_allocator_fixed_t_pop,
	}
};

