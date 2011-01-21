#include <libfrozen.h>
#include <backends/mphf/mphf.h>
#include <backends/mphf/mphf_chm_imp.h>
#include <backends/mphf/mphf_bdz_imp.h>
#include <backends/mphf/hash_jenkins.h>

#define MAX_STORES_PER_MPHF  10000
#define N_INITIAL_DEFAULT    256
#define VALUE_BITS_DEFAULT   31

typedef struct mphf_userdata {
	mphf_t       main_mphf;
	backend_t   *backend;
	hash_key_t   key_from;
	hash_key_t   key_to;
	unsigned int inited;
} mphf_userdata;

/* mphf {{{ */
ssize_t         mphf_new         (mphf_t *mphf, backend_t *backend, uint64_t *offset, mphf_types type, uint64_t nelements, uint32_t value_bits){
	ssize_t   ret;
	uint32_t  nstores;
	uint64_t  mphf_stores_size, mphf_size;
	buffer_t  mphf_buffer;
	
	// calc space for stores
	switch(type){
		case MPHF_TYPE_CHM_IMP: nstores = mphf_chm_imp_nstores(nelements, value_bits); break;
		case MPHF_TYPE_BDZ_IMP: nstores = mphf_bdz_imp_nstores(nelements, value_bits); break;
		case MPHF_TYPE_INVALID:
			return -EINVAL;
		default: nstores = 0; break;
	};
	if(nstores >= MAX_STORES_PER_MPHF)
		return -EINVAL;
	
	// calc sizes
	mphf_stores_size = nstores * sizeof(mphf_store_t);  
	mphf_size        = sizeof(mphf_params_t) + mphf_stores_size;
	
	// fill struct
	mphf->backend        = backend;
	mphf->params.type    = (uint32_t)type;
	mphf->params.nstores = nstores;
	mphf->stores         = calloc(1, mphf_stores_size);
	
	if(mphf->stores == NULL)
		goto error2;
	
	// reserve space for mphf
	*offset = 0;
	request_t  r_create[] = {
		{ HK(action),     DATA_INT32(ACTION_CRWD_CREATE)      },
		{ HK(offset_out), DATA_PTR_INT64(offset)              },
		{ HK(size),       DATA_PTR_INT64(&mphf_size)          },
		hash_end
	};
	if(backend_query(backend, r_create) < 0)
		goto error;
	
	// algo dependent
	switch(type){
		case MPHF_TYPE_CHM_IMP: ret = mphf_chm_imp_new(mphf, nelements, value_bits); break;
		case MPHF_TYPE_BDZ_IMP: ret = mphf_bdz_imp_new(mphf, nelements, value_bits); break;
		default:                ret = -1; break;
	};
	if(ret < 0)
		goto error;
	
	// write mphf
	buffer_init(&mphf_buffer);
	buffer_add_tail_raw(&mphf_buffer, &mphf->params, sizeof(mphf_params_t));
	buffer_add_tail_raw(&mphf_buffer, mphf->stores,  mphf_stores_size);
	
	request_t  r_write[] = {
		{ HK(action),  DATA_INT32(ACTION_CRWD_WRITE)       },
		{ HK(offset),  DATA_PTR_INT64(offset)              },
		{ HK(buffer),  DATA_BUFFERT(&mphf_buffer)          },
		{ HK(size),    DATA_PTR_INT64(&mphf_size)          },
		hash_end
	};
	ret = backend_query(backend, r_write);
	buffer_destroy(&mphf_buffer);
	
	if(ret < 0)
		goto error;
	
	return 0;
error:
	free(mphf->stores);
error2:
	return -EFAULT;
}

ssize_t         mphf_load        (mphf_t *mphf, backend_t *backend, uint64_t offset){
	ssize_t   ret;
	uint64_t  mphf_stores_size, mphf_size;
	buffer_t  mphf_buffer;
	
	mphf->backend = backend;
	
	memset(&mphf->params, 0, sizeof(mphf_params_t));
	
	// pre-read
	request_t  r_preread[] = {
		{ HK(action),  DATA_INT32(ACTION_CRWD_READ)                    },
		{ HK(offset),  DATA_PTR_INT64(&offset)                         },
		{ HK(buffer),  DATA_RAW(&mphf->params, sizeof(mphf_params_t))  },
		{ HK(size),    DATA_INT64(sizeof(mphf_params_t))               },
		hash_end
	};
	if(backend_query(backend, r_preread) <= 0)
		goto error;
	
	switch(mphf->params.type){
		case MPHF_TYPE_CHM_IMP:
		case MPHF_TYPE_BDZ_IMP:
			break;
		default:
			return -EINVAL;
	}
	
	// malloc
	if(mphf->params.nstores >= MAX_STORES_PER_MPHF)
		return -EINVAL;
	
	mphf_stores_size = mphf->params.nstores * sizeof(mphf_store_t);
	mphf_size        = sizeof(mphf_params_t) + mphf_stores_size;
	
	mphf->stores = malloc(mphf_stores_size);
	
	// read
	buffer_init(&mphf_buffer);
	buffer_add_tail_raw(&mphf_buffer, &mphf->params, sizeof(mphf_params_t));
	buffer_add_tail_raw(&mphf_buffer, mphf->stores,  mphf_stores_size);
	
	request_t  r_read[] = {
		{ HK(action),  DATA_INT32(ACTION_CRWD_READ)        },
		{ HK(offset),  DATA_PTR_INT64(&offset)             },
		{ HK(buffer),  DATA_BUFFERT(&mphf_buffer)          },
		hash_end
	};
	ret = backend_query(backend, r_read);
	buffer_destroy(&mphf_buffer);
	
	if(ret < 0)
		goto error;
	
	return 0;
error:
	return -EFAULT;
}

ssize_t         mphf_insert      (mphf_t *mphf, char *key, size_t keylen, uint64_t  value){
	switch(mphf->params.type){
		case MPHF_TYPE_CHM_IMP: return mphf_chm_imp_insert(mphf, key, keylen, value);
		case MPHF_TYPE_BDZ_IMP: return mphf_bdz_imp_insert(mphf, key, keylen, value);
	}
	return -EFAULT;
}

ssize_t         mphf_query       (mphf_t *mphf, char *key, size_t keylen, uint64_t *value){
	switch(mphf->params.type){
		case MPHF_TYPE_CHM_IMP: return mphf_chm_imp_query(mphf, key, keylen, value);
		case MPHF_TYPE_BDZ_IMP: return mphf_chm_imp_query(mphf, key, keylen, value);
	}
	return -EFAULT;
}

void            mphf_free        (mphf_t *mphf){
	free(mphf->stores);
}

mphf_types      mphf_string_to_type(char *string){
	if(string == NULL)                 return MPHF_TYPE_INVALID;
	if(strcmp(string, "chm_imp") == 0) return MPHF_TYPE_CHM_IMP;
	if(strcmp(string, "bdz_imp") == 0) return MPHF_TYPE_BDZ_IMP;
	return MPHF_TYPE_INVALID;
}

/* }}} */
/* store {{{ */
ssize_t         mphf_store_new   (mphf_t *mphf, uint32_t id, uint64_t size){
	if(id >= mphf->params.nstores)
		return -EINVAL;
	
	mphf_store_t *store = &mphf->stores[id];
	
	request_t  r_create[] = {
		{ HK(action),     DATA_INT32(ACTION_CRWD_CREATE)     },
		{ HK(size),       DATA_PTR_INT64(&size)              },
		{ HK(offset_out), DATA_PTR_INT64(&store->offset)     },
		hash_end
	};
	
	return backend_query(mphf->backend, r_create);
}

ssize_t         mphf_store_read  (mphf_t *mphf, uint32_t id, uint64_t offset, void *buffer, size_t buffer_size){
	if(id >= mphf->params.nstores)
		return -EINVAL;
	
	mphf_store_t *store = &mphf->stores[id];
	
	uint64_t foffset = store->offset + offset;
	request_t  r_read[] = {
		{ HK(action),  DATA_INT32(ACTION_CRWD_READ)   },
		{ HK(offset),  DATA_PTR_INT64(&foffset)       },
		{ HK(buffer),  DATA_RAW(buffer, buffer_size)  },
		{ HK(size),    DATA_PTR_SIZET(&buffer_size)   },
		hash_end
	};
	
	return backend_query(mphf->backend, r_read);
}

ssize_t         mphf_store_write (mphf_t *mphf, uint32_t id, uint64_t offset, void *buffer, size_t buffer_size){
	if(id >= mphf->params.nstores)
		return -EINVAL;
	
	mphf_store_t *store = &mphf->stores[id];
	
	uint64_t foffset = store->offset + offset;
	request_t  r_write[] = {
		{ HK(action),  DATA_INT32(ACTION_CRWD_WRITE)  },
		{ HK(offset),  DATA_PTR_INT64(&foffset)       },
		{ HK(buffer),  DATA_RAW(buffer, buffer_size)  },
		{ HK(size),    DATA_PTR_SIZET(&buffer_size)   },
		hash_end
	};
	
	return backend_query(mphf->backend, r_write);
}

/* }}} */
/* hashes {{{ */
uint32_t        mphf_hash32      (mphf_hash_types type, uint32_t seed, void *key, size_t key_size, uint32_t *hashes[], size_t nhashes){
	switch(type){
		case MPHF_HASH_JENKINS32: return jenkins32_hash(seed, key, key_size, hashes, nhashes);
	};
	return 0;
}
/* }}} */

static int mphf_init(chain_t *chain){ // {{{
	mphf_userdata *userdata = chain->userdata = calloc(1, sizeof(mphf_userdata));
	if(userdata == NULL)
		return -ENOMEM;
	
	return 0;
} // }}}
static int mphf_destroy(chain_t *chain){ // {{{
	mphf_userdata *userdata = (mphf_userdata *)chain->userdata;
	
	backend_destroy(userdata->backend);
	if(userdata->inited == 1)
		mphf_free(&userdata->main_mphf);
	free(userdata);
	return 0;
} // }}}
static int mphf_configure(chain_t *chain, hash_t *config){ // {{{
	DT_INT64         n_initial      = N_INITIAL_DEFAULT;
	DT_INT32         value_bits     = VALUE_BITS_DEFAULT;
	DT_STRING        mphf_type_str  = NULL;
	DT_STRING        key_from_str   = "key";
	DT_STRING        key_to_str     = "offset";
	data_t          *backend_name;
	ssize_t          ret;
	uint64_t         offset;
	mphf_types       mphf_type;
	mphf_userdata   *userdata = (mphf_userdata *)chain->userdata;
	
	hash_copy_data(ret, TYPE_INT64,  n_initial,     config, HK(n_initial));
	hash_copy_data(ret, TYPE_INT32,  value_bits,    config, HK(value_bits));
	hash_copy_data(ret, TYPE_STRING, mphf_type_str, config, HK(type));
	hash_copy_data(ret, TYPE_STRING, key_from_str,  config, HK(key_from));
	hash_copy_data(ret, TYPE_STRING, key_to_str,    config, HK(key_to));
	
	if( (backend_name = hash_get_typed_data(config, TYPE_STRING, HK(backend))) == NULL)
		return_error(-EINVAL, "chain 'mphf' parameter 'backend' not supplied\n");
	
	userdata->backend  = backend_acquire(backend_name);
	userdata->key_from = hash_string_to_key(key_from_str);
	userdata->key_to   = hash_string_to_key(key_to_str);
	
	// try load
	if(mphf_load(&userdata->main_mphf, userdata->backend, 0) == 0)
		return 0;
	
	// create new
	if( (mphf_type = mphf_string_to_type(mphf_type_str)) == MPHF_TYPE_INVALID)
		return_error(-EINVAL, "chain 'mphf' parameter 'mphf_type' invalid or not supplied\n");
	
	if(mphf_new(&userdata->main_mphf, userdata->backend, &offset, mphf_type, n_initial, value_bits) != 0)
		return_error(-EFAULT, "chain 'mphf' mphf_new failed\n");
	
	userdata->inited = 1;
	
	return 0;
} // }}}

static ssize_t mphf_backend_insert(chain_t *chain, request_t *request){
	ssize_t          ret;
	data_t          *key;
	uint64_t         key_out;
	mphf_userdata   *userdata = (mphf_userdata *)chain->userdata;
	
	if( (key = hash_get_data(request, userdata->key_from)) == NULL)
		return chain_next_query(chain, request);
	
	request_t r_next[] = {
		{ HK(offset_out),      DATA_PTR_INT64(&key_out) },
		hash_next(request)
	};
	
	if( (ret = chain_next_query(chain, r_next)) < 0)
		return ret;
	
	if(mphf_insert(&userdata->main_mphf, data_value_ptr(key), data_value_len(key), key_out) < 0)
		return -EFAULT;
	
	return 0;
}

static ssize_t mphf_backend_query(chain_t *chain, request_t *request){
	data_t          *key;
	uint64_t         key_out;
	mphf_userdata   *userdata = (mphf_userdata *)chain->userdata;
	
	if( (key = hash_get_data(request, userdata->key_from)) == NULL)
		return chain_next_query(chain, request);
	
	switch(mphf_query(&userdata->main_mphf, data_value_ptr(key), data_value_len(key), &key_out)){
		case MPHF_QUERY_NOTFOUND: return -EBADF;
		case MPHF_QUERY_FOUND:;
			request_t r_next[] = {
				{ userdata->key_to, DATA_PTR_INT64(&key_out) },
				hash_next(request)
			};
			return chain_next_query(chain, r_next);
	}
	return -EFAULT;
}

static ssize_t mphf_backend_queryinsert(chain_t *chain, request_t *request){
	ssize_t ret;
	
	if((ret = mphf_backend_query(chain, request)) == -EBADF){
		return mphf_backend_insert(chain, request);
	}
	return ret;
}

static chain_t chain_mphf = {
	"mphf",
	CHAIN_TYPE_CRWD,
	.func_init      = &mphf_init,
	.func_configure = &mphf_configure,
	.func_destroy   = &mphf_destroy,
	{{
		.func_create = &mphf_backend_insert,
		.func_set    = &mphf_backend_queryinsert,
		.func_get    = &mphf_backend_query,
		.func_delete = &mphf_backend_query
	}}
};
CHAIN_REGISTER(chain_mphf)

