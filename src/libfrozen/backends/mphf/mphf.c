#include <libfrozen.h>
#include <backends/mphf/mphf.h>
#include <backends/mphf/hash_jenkins.h>

/* store {{{ */
mphf_store_t *  mphf_store_new   (mphf_t *mphf, uint64_t size){
	mphf_store_t *store = malloc(sizeof(mphf_store_t));
	
	store->mphf    = mphf;
	store->size    = size;
	
	request_t  r_create[] = {
		{ "action",  DATA_INT32(ACTION_CRWD_CREATE) },
		{ "size",    DATA_PTR_INT64(&size)          },
		{ "key_out", DATA_PTR_INT64(&store->offset) },
		hash_end
	};
	if(backend_query(mphf->data_backend, r_create) < 0)
		goto error;
	
	if(
		size < TYPE_MAX(size_t) &&
		(
			mphf->memory_limit == 0 ||
			size <= (mphf->memory_limit - mphf->memory_consumed)
		)
	){
		if( (store->cache = malloc((size_t)size)) == NULL)
			goto exit;
		
		mphf->memory_consumed += (size_t)size;	
	}else{
		store->cache = NULL;
	}
exit:
	return store;
error:
	free(store);
	return NULL;
}

ssize_t         mphf_store_read  (mphf_store_t *store, uint64_t offset, void *buffer, size_t buffer_size){
	if(store->cache){
		memcpy(buffer, store->cache + offset, buffer_size);
		return 0;
	}
	
	uint64_t foffset = store->offset + offset;
	request_t  r_read[] = {
		{ "action",  DATA_INT32(ACTION_CRWD_READ)   },
		{ "offset",  DATA_PTR_INT64(&foffset)       },
		{ "buffer",  DATA_MEM(buffer, buffer_size)  },
		{ "size",    DATA_PTR_INT64(&buffer_size)   },
		hash_end
	};
	return backend_query(store->mphf->data_backend, r_read);
}

ssize_t         mphf_store_write (mphf_store_t *store, uint64_t offset, void *buffer, size_t buffer_size){
	if(store->cache){
		memcpy(store->cache + offset, buffer, buffer_size);
		return 0;
	}
	
	uint64_t foffset = store->offset + offset;
	request_t  r_write[] = {
		{ "action",  DATA_INT32(ACTION_CRWD_WRITE)  },
		{ "offset",  DATA_PTR_INT64(&foffset)       },
		{ "buffer",  DATA_MEM(buffer, buffer_size)  },
		{ "size",    DATA_PTR_INT64(&buffer_size)   },
		hash_end
	};
	return backend_query(store->mphf->data_backend, r_write);
}

void            mphf_store_free  (mphf_store_t *store){
	void *cache;
	
	if(store->cache){
		cache        = store->cache;
		store->cache = NULL;
		mphf_store_write(store, 0, cache, (size_t)store->size);
		
		free(cache);
	}
	
	free(store);
}
/* }}} */
/* hashes {{{ */
uint32_t        mphf_hash32      (mphf_hash_types type, uint32_t seed, void *key, size_t key_size){
	switch(type){
		case MPHF_HASH_JENKINS32: return jenkins32_hash(seed, key, key_size);
	};
	return 0;
}
/* }}} */

static int mphf_init(chain_t *chain){
	(void)chain;
	return 0;
}

static int mphf_destroy(chain_t *chain){
	(void)chain;
	return 0;
}

static int mphf_configure(chain_t *chain, hash_t *config){
	(void)chain; (void)config;
	return 0;
}

static ssize_t mphf_create(chain_t *chain, request_t *request){
	return -1;
}

static chain_t chain_mphf = {
	"mphf",
	CHAIN_TYPE_CRWD,
	.func_init      = &mphf_init,
	.func_configure = &mphf_configure,
	.func_destroy   = &mphf_destroy,
	{{
		.func_create = &mphf_create,
	}}
};
CHAIN_REGISTER(chain_mphf)

