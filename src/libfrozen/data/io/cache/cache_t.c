#include <libfrozen.h>
#include <cache_t.h>

#include <core/void/void_t.h>
#include <core/hash/hash_t.h>
#include <enum/format/format_t.h>
#include <numeric/uint/uint_t.h>

#define CHUNK_SIZE_DEFAULT 32768
#define NSLOTS_DEFAULT     16

typedef struct cache_slot_t {
	uintmax_t              offset;        // offset in storage for this chunk of data
	uintmax_t              size;          // actual size of data inside chunk, no more than <chunk_size>; == 0 - free slot
	void                  *buffer;        // pre-allocated chunk of size <chunk_size>
} cache_slot_t;

typedef struct cache_t {
	data_t                 storage;
	uintmax_t              chunk_size;
	
	uintmax_t              lru;
	uintmax_t              nslots;
	cache_slot_t          *slots;
} cache_t;

ssize_t          cache_pass_request(cache_t *fdata, fastcall_header *hargs){ // {{{
	return data_query(&fdata->storage, hargs);
} // }}}

ssize_t          cache_slot_alloc(cache_slot_t *slot, uintmax_t chunk_size){ // {{{
	if( (slot->buffer = malloc(chunk_size)) == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
void             cache_slot_free(cache_slot_t *slot){ // {{{
	if(slot->buffer)
		free(slot->buffer);
	
	slot->offset = 0;
	slot->size   = 0;
	slot->buffer = NULL;
} // }}}
void             cache_slot_invalidate(cache_slot_t *slot){ // {{{
	slot->offset = 0;
	slot->size   = 0;
} // }}}
ssize_t          cache_slot_fill(cache_t *fdata, cache_slot_t *slot, uintmax_t offset){ // {{{
	ssize_t                ret;
	
	fastcall_read r_read = { { 5, ACTION_READ }, offset, slot->buffer, fdata->chunk_size };
	if( (ret = cache_pass_request(fdata, (fastcall_header *)&r_read)) < 0)
		return ret;
	
	slot->offset = offset;
	slot->size   = r_read.buffer_size;
	return 0;
} // }}}

void             cache_slots_free(cache_t *fdata){ // {{{
	uintmax_t              i;
	cache_slot_t          *slot;
	
	for(i = 0, slot = fdata->slots; i < fdata->nslots; i++, slot++)
		cache_slot_free(slot);
	
	free(fdata->slots);
} // }}}
ssize_t          cache_slots_alloc(cache_t *fdata, uintmax_t nslots, uintmax_t chunk_size){ // {{{
	ssize_t                ret;
	uintmax_t              i;
	cache_slot_t          *slot;
	
	if( __MAX(uintmax_t) / sizeof(cache_slot_t) <= nslots)
		return -EINVAL;
	
	if( (fdata->slots = calloc(sizeof(cache_slot_t), nslots)) == NULL)
		return -ENOMEM;
	
	fdata->chunk_size = chunk_size;
	fdata->nslots     = nslots;
	fdata->lru        = 0;
	
	for(i = 0, slot = fdata->slots; i < fdata->nslots; i++, slot++){
		if( (ret = cache_slot_alloc(slot, fdata->chunk_size)) < 0)
			goto error;
	}
	return 0;
error:
	cache_slots_free(fdata);
	return ret;
} // }}}
cache_slot_t *   cache_slots_use(cache_t *fdata){ // {{{
	fdata->lru = (fdata->lru + 1) % fdata->nslots;
	return &(fdata->slots[fdata->lru]);
} // }}}
cache_slot_t *   cache_slots_find(cache_t *fdata, uintmax_t offset){ // {{{
	uintmax_t              i;
	cache_slot_t          *slot;
	
	for(i = 0, slot = fdata->slots; i < fdata->nslots; i++, slot++){
		if(slot->size == 0)
			continue;
		
		if(slot->offset <= offset && offset - slot->offset < slot->size)
			return slot;
	}
	return NULL;
} // }}}

ssize_t          cache_read(cache_t *fdata, fastcall_read *fargs){ // {{{
	ssize_t                ret;
	uintmax_t              hit_offset;
	uintmax_t              hit_size;
	uintmax_t              offset;
	void                  *buffer;
	uintmax_t              buffer_size;
	cache_slot_t          *slot;
	
	// for simplicity - skip reads bigger than cache size
	if(fargs->buffer_size > fdata->chunk_size)
		return cache_pass_request(fdata, (fastcall_header *)fargs);
	
	offset      = fargs->offset;
	buffer      = fargs->buffer;
	buffer_size = fargs->buffer_size;
	fargs->buffer_size = 0;
	do{
		// find hit
		if( (slot = cache_slots_find(fdata, offset)) == NULL){
			// not found
			// - use new
			if( (slot = cache_slots_use(fdata)) == NULL)
				return -EFAULT;
			
			// - read to our cache <chunk_size>
			if( (ret = cache_slot_fill(fdata, slot, offset)) < -1)
				return ret;
			
			if(ret == -1){
				if(offset == fargs->offset) // nothing was read, return EOF
					return ret;
				
				return 0; // something was read, not yet EOF
			}
			
		}
		
		// found, or created new slot already. Read data from cache to user's buffer
		hit_offset = ( offset - slot->offset );
		hit_size   = MIN(slot->size - hit_offset, buffer_size);
		memcpy(
			buffer,
			slot->buffer + hit_offset,
			hit_size
		);
		offset             += hit_size;
		buffer             += hit_size;
		buffer_size        -= hit_size;
		fargs->buffer_size += hit_size;
	}while(buffer_size > 0);
	return 0;
} // }}}

ssize_t data_cache_t(data_t *data, data_t storage, uintmax_t nslots, uintmax_t chunk_size){ // {{{
	ssize_t                ret;
	cache_t               *fdata;
	
	if( (fdata = malloc(sizeof(cache_t))) == NULL)
		return -ENOMEM;
	
	fdata->storage    = storage;
	if( (ret = cache_slots_alloc(fdata, nslots, chunk_size)) < 0){
		free(fdata);
		return ret;
	}
	
	data->type = TYPE_CACHET;
	data->ptr  = fdata;
	return 0;
} // }}}
ssize_t data_cache_t_from_config(data_t *data, config_t *config){ // {{{
	ssize_t                ret;
	uintmax_t              chunk_size        = CHUNK_SIZE_DEFAULT;
	uintmax_t              nslots            = NSLOTS_DEFAULT;
	data_t                 storage           = DATA_VOID;
	
	hash_holder_consume(ret, storage, config, HK(data));
	
	hash_data_get(ret, TYPE_UINTT, chunk_size, config, HK(chunk_size));
	hash_data_get(ret, TYPE_UINTT, nslots,     config, HK(nslots));
	
	return data_cache_t(data, storage, nslots, chunk_size);
} // }}}
void    data_cache_t_destroy(data_t *data){ // {{{
	cache_t               *fdata             = (cache_t *)data->ptr;
	
	if(fdata){
		cache_slots_free(fdata);
		data_free(&fdata->storage);
		free(fdata);
	}
	data_set_void(data);
} // }}}

static ssize_t data_cache_t_convert_from(data_t *dst, fastcall_convert_from *fargs){ // {{{
	ssize_t                ret;
	cache_t               *fdata             = (cache_t *)dst->ptr;
	
	switch(fargs->format){
		case FORMAT(hash):;
			hash_t                *config;
			
			if(fdata != NULL)
				return -EINVAL;
			
			data_get(ret, TYPE_HASHT, config, fargs->src);
			if(ret != 0)
				return -EINVAL;
			
			return data_cache_t_from_config(dst, config);
			
		default:
			break;
	}
	return -ENOSYS;
} // }}}
static ssize_t data_cache_t_free(data_t *data, fastcall_free *fargs){ // {{{
	data_cache_t_destroy(data);
	return 0;
} // }}}


static ssize_t data_cache_t_default(data_t *data, fastcall_header *hargs){ // {{{
	cache_t               *fdata             = (cache_t *)data->ptr;
	return cache_pass_request(fdata, hargs);
} // }}}
static ssize_t data_cache_t_read(data_t *data, fastcall_read *fargs){ // {{{
	cache_t               *fdata             = (cache_t *)data->ptr;
	return cache_read(fdata, fargs);
} // }}}
static ssize_t data_cache_t_write(data_t *data, fastcall_write *fargs){ // {{{
	cache_t               *fdata             = (cache_t *)data->ptr;
	return data_query(&fdata->storage, fargs);
} // }}}

data_proto_t cache_t_proto = {
	.type                   = TYPE_CACHET,
	.type_str               = "cache_t",
	.api_type               = API_HANDLERS,
	.properties             = DATA_PROXY,
	.handler_default        = (f_data_func)&data_cache_t_default,
	.handlers               = {
		[ACTION_CONVERT_FROM] = (f_data_func)&data_cache_t_convert_from,
		[ACTION_FREE]         = (f_data_func)&data_cache_t_free,
		[ACTION_CONTROL]      = (f_data_func)&data_default_control,
		
		[ACTION_READ]         = (f_data_func)&data_cache_t_read,
		[ACTION_WRITE]        = (f_data_func)&data_cache_t_write,
		
		[ACTION_LOOKUP]       = (f_data_func)&data_default_lookup,
		[ACTION_ENUM]         = (f_data_func)&data_default_enum,
	}
};
