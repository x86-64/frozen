#define MPHF_C

#include <libfrozen.h>
#include <backends/mphf/mphf.h>

#define MAX_STORES_PER_MPHF  10000

typedef enum mphf_build_control {
	BUILD_ONLOAD,
	BUILD_ONREQUEST,
	//BUILD_ONTIMER
	
	BUILD_INVALID = -1
} mphf_build_control;

typedef struct mphf_userdata {
	mphf_t               mphf;
	mphf_proto_t        *mphf_proto;
	mphf_build_control   build_start;
	mphf_build_control   build_end;
	uint32_t             in_build;
	
	hash_key_t           key_from;
	hash_key_t           key_to;
} mphf_userdata;

mphf_proto_t *       mphf_string_to_proto(char *string){ // {{{
	if(string == NULL)                 return NULL;
	if(strcmp(string, "chm_imp") == 0) return &mphf_protos[MPHF_TYPE_CHM_IMP];
	if(strcmp(string, "bdz_imp") == 0) return &mphf_protos[MPHF_TYPE_BDZ_IMP];
	return NULL;
} // }}}
mphf_hash_proto_t *  mphf_string_to_hash_proto(char *string){ // {{{
	if(string == NULL)                 return NULL;
	if(strcmp(string, "jenkins") == 0) return &mphf_hash_protos[MPHF_HASH_JENKINS32];
	return NULL;
} // }}}
mphf_build_control   mphf_string_to_build_control(char *string){ // {{{
	if(string != NULL){
		if(strcmp(string, "onload")       == 0 ||
		   strcmp(string, "onunload")     == 0) return BUILD_ONLOAD;
		if(strcmp(string, "onrequest")    == 0 ||
		   strcmp(string, "onrequestend") == 0) return BUILD_ONREQUEST;
	}
	return BUILD_INVALID;
} // }}}

static ssize_t mphf_build_start(mphf_userdata *userdata, mphf_build_control event){ // {{{
	if(userdata->build_start == event){
		if(userdata->mphf_proto->func_build_start(&userdata->mphf) < 0)
			return -EFAULT;
		
		userdata->in_build = 1;
	}
	return 0;
} // }}}
static ssize_t mphf_build_end(mphf_userdata *userdata, mphf_build_control event){ // {{{
	if(userdata->in_build == 1 && userdata->build_end == event){
		if(userdata->mphf_proto->func_build_end(&userdata->mphf) < 0)
			return -EFAULT;
		
		userdata->in_build = 0;
	}
	return 0;
} // }}}

/* store {{{ */
ssize_t         mphf_store_new   (backend_t *backend, uint64_t *offset, uint64_t size){ // {{{
	request_t  r_create[] = {
		{ HK(action),     DATA_INT32(ACTION_CRWD_CREATE)     },
		{ HK(size),       DATA_PTR_INT64(&size)              },
		{ HK(offset_out), DATA_PTR_INT64(offset)             },
		hash_end
	};
	
	return backend_query(backend, r_create);
} // }}}
ssize_t         mphf_store_read  (backend_t *backend, uint64_t  offset, void *buffer, size_t buffer_size){ // {{{
	request_t  r_read[] = {
		{ HK(action),  DATA_INT32(ACTION_CRWD_READ)   },
		{ HK(offset),  DATA_PTR_INT64(&offset)        },
		{ HK(buffer),  DATA_RAW(buffer, buffer_size)  },
		{ HK(size),    DATA_PTR_SIZET(&buffer_size)   },
		hash_end
	};
	
	return backend_query(backend, r_read);
} // }}}
ssize_t         mphf_store_write (backend_t *backend, uint64_t  offset, void *buffer, size_t buffer_size){ // {{{
	request_t  r_write[] = {
		{ HK(action),  DATA_INT32(ACTION_CRWD_WRITE)  },
		{ HK(offset),  DATA_PTR_INT64(&offset)        },
		{ HK(buffer),  DATA_RAW(buffer, buffer_size)  },
		{ HK(size),    DATA_PTR_SIZET(&buffer_size)   },
		hash_end
	};
	
	return backend_query(backend, r_write);
} // }}}
ssize_t         mphf_store_fill  (backend_t *backend, uint64_t  offset, void *buffer, size_t buffer_size, size_t fill_size){ // {{{
	size_t size;
	
	do{
		size = MIN(fill_size, buffer_size);
		mphf_store_write(backend, offset, buffer, size);
		
		offset    += size;
		fill_size -= size;
	}while(fill_size > 0);
	return 0;
} // }}}
/* }}} */
/* hashes {{{ */
uint32_t        mphf_hash32      (mphf_hash_types type, uint32_t seed, void *key, size_t key_size, uint32_t hashes[], size_t nhashes){
	if(type >= mphf_hash_protos_size)
		return 0;
	
	return mphf_hash_protos[type].func_hash32(seed, key, key_size, hashes, nhashes);
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
	
	mphf_build_end(userdata, BUILD_ONLOAD);
	
	backend_destroy (userdata->mphf.backend);
	hash_free       (userdata->mphf.config);
	free(userdata);
	return 0;
} // }}}
static int mphf_configure(chain_t *chain, hash_t *config){ // {{{
	DT_INT64         count           = 0;
	DT_STRING        mphf_type_str   = NULL;
	DT_STRING        key_from_str    = "key";
	DT_STRING        key_to_str      = "offset";
	DT_STRING        build_start_str = NULL;
	DT_STRING        build_end_str   = NULL;
	data_t          *backend_name;
	ssize_t          ret;
	mphf_userdata   *userdata = (mphf_userdata *)chain->userdata;
	
	hash_copy_data(ret, TYPE_STRING, mphf_type_str,   config, HK(type));
	hash_copy_data(ret, TYPE_STRING, key_from_str,    config, HK(key_from));
	hash_copy_data(ret, TYPE_STRING, key_to_str,      config, HK(key_to));
	hash_copy_data(ret, TYPE_STRING, build_start_str, config, HK(build_start));
	hash_copy_data(ret, TYPE_STRING, build_end_str,   config, HK(build_end));
	
	if( (backend_name = hash_get_typed_data(config, TYPE_STRING, HK(backend))) == NULL)
		return_error(-EINVAL, "chain 'mphf' parameter 'backend' not supplied\n");
	
	if( (userdata->mphf_proto = mphf_string_to_proto(mphf_type_str)) == NULL)
		return_error(-EINVAL, "chain 'mphf' parameter 'mphf_type' invalid or not supplied\n");
	
	if( (userdata->build_start = mphf_string_to_build_control(build_start_str)) == BUILD_INVALID)
		return_error(-EINVAL, "chain 'mphf' parameter 'build_start' invalid\n");
	if( (userdata->build_end   = mphf_string_to_build_control(build_end_str))   == BUILD_INVALID)
		return_error(-EINVAL, "chain 'mphf' parameter 'build_end' invalid\n");
	
	memset(&userdata->mphf, 0, sizeof(userdata->mphf));
	if( (userdata->mphf.backend = backend_acquire(backend_name)) == NULL)
		return_error(-EINVAL, "chain 'mphf' parameter 'backend' invalid\n");
	
	if( (userdata->mphf.config  = hash_copy(config)) == NULL)
		return -ENOMEM;
	
	userdata->mphf.offset     = 0;
	userdata->mphf.build_data = NULL;
	userdata->key_from        = hash_string_to_key(key_from_str);
	userdata->key_to          = hash_string_to_key(key_to_str);
	
	request_t r_count[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_COUNT) },
		{ HK(buffer), DATA_PTR_INT64(&count)        },
		hash_end
	};
	backend_query(userdata->mphf.backend, r_count);
	
	if(count == 0 && userdata->mphf_proto->func_new(&userdata->mphf) < 0)
		return -EFAULT;
	
	if(mphf_build_start(userdata, BUILD_ONLOAD) < 0)
		return -EFAULT;
	
	return 0;
} // }}}

static ssize_t mphf_backend_insert(chain_t *chain, request_t *request){ // {{{
	ssize_t          ret;
	data_t          *key;
	off_t            key_out;
	mphf_userdata   *userdata = (mphf_userdata *)chain->userdata;
	
	if( (key = hash_get_data(request, userdata->key_from)) == NULL)
		return chain_next_query(chain, request);
	
	// start build
	if(mphf_build_start(userdata, BUILD_ONREQUEST) < 0)
		return -EFAULT;
	
	// get new key_out
	request_t r_next[] = {
		{ HK(offset_out),      DATA_PTR_OFFT(&key_out) },
		hash_next(request)
	};
	if( (ret = chain_next_query(chain, r_next)) < 0)
		goto error;
	
	// insert item
	if(userdata->mphf_proto->func_insert(
		&userdata->mphf, 
		data_value_ptr(key),
		data_value_len(key),
		key_out
	) < 0){ ret = -EFAULT; goto error; }
	
	ret = 0;
error:
	// stop build
	if(mphf_build_end(userdata, BUILD_ONREQUEST) < 0)
		ret = -EFAULT;
	
	return ret;
} // }}}
static ssize_t mphf_backend_query(chain_t *chain, request_t *request){ // {{{
	data_t          *key;
	uint64_t         key_out;
	mphf_userdata   *userdata = (mphf_userdata *)chain->userdata;
	
	if( (key = hash_get_data(request, userdata->key_from)) == NULL)
		return chain_next_query(chain, request);
	
	switch(userdata->mphf_proto->func_query(&userdata->mphf, data_value_ptr(key), data_value_len(key), &key_out)){
		case MPHF_QUERY_NOTFOUND: return -EBADF;
		case MPHF_QUERY_FOUND:;
			request_t r_next[] = {
				{ userdata->key_to, DATA_PTR_OFFT(&key_out) },
				hash_next(request)
			};
			return chain_next_query(chain, r_next);
	}
	return 0;
} // }}}
static ssize_t mphf_backend_queryinsert(chain_t *chain, request_t *request){ // {{{
	ssize_t ret;
	
	if((ret = mphf_backend_query(chain, request)) == -EBADF){
		return mphf_backend_insert(chain, request);
	}
	return ret;
} // }}}

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

