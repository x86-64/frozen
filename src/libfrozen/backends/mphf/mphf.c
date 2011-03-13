#define MPHF_C

#include <libfrozen.h>
#include <backends/mphf/mphf.h>

#define BUFFER_SIZE_DEFAULT  1000

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
	chain_t             *backend_data;
	uint32_t             in_build;
	size_t               buffer_size;
	
	hash_key_t           key_from;
	hash_key_t           key_to;
	hash_key_t           keyid;
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
ssize_t         mphf_store_delete (backend_t *backend, uint64_t  offset, uint64_t size){ // {{{
	request_t  r_delete[] = {
		{ HK(action),     DATA_INT32(ACTION_CRWD_DELETE)     },
		{ HK(offset),     DATA_PTR_INT64(&offset)            },
		{ HK(size),       DATA_PTR_INT64(&size)              },
		hash_end
	};
	
	return backend_query(backend, r_delete);
} // }}}
/* }}} */
/* hashes {{{ */
uint32_t        mphf_hash32      (mphf_hash_types type, uint32_t seed, void *key, size_t key_size, uint32_t hashes[], size_t nhashes){
	if(type >= mphf_hash_protos_size)
		return 0;
	
	return mphf_hash_protos[type].func_hash32(seed, key, key_size, hashes, nhashes);
}
/* }}} */

static uint64_t mphf_key_str_to_key(mphf_t *mphf, char *key, size_t key_len){ // {{{
	uint64_t value = 0;
	
	// TODO 64-bit
	mphf->hash->func_hash32(0, key, key_len, (uint32_t *)&value, 1);
	return value;
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
static ssize_t mphf_insert(mphf_userdata *userdata, uint64_t keyid, uint64_t value){ // {{{
	ssize_t   ret;
	
	// start build
	if(mphf_build_start(userdata, BUILD_ONREQUEST) < 0)
		return -EFAULT;
	
	// insert item
	if( (ret = userdata->mphf_proto->func_insert(
		&userdata->mphf, 
		keyid,
		value
	)) < 0)
		goto error;
	
	ret = 0;
error:
	// stop build
	if(mphf_build_end(userdata, BUILD_ONREQUEST) < 0)
		ret = -EFAULT;
	
	return ret;
} // }}}
static ssize_t mphf_rebuild(mphf_userdata *userdata){ // {{{
	ssize_t   ret;
	DT_OFFT   i;
	DT_OFFT   count = 0;
	char     *buffer;
	data_t   *key;
	uint64_t  keyid;
	
	// count elements
	request_t r_count[] = {
		{ HK(action), DATA_INT32(ACTION_CRWD_COUNT) },
		{ HK(buffer), DATA_PTR_OFFT(&count)         },
		hash_end
	};
	if(chain_next_query(userdata->backend_data, r_count) < 0)
		return 0;
	
	// prepare buffer
	if( (buffer = malloc(userdata->buffer_size)) == NULL)
		return -EFAULT;
	
redo:
	mphf_build_end(userdata, BUILD_ONLOAD);
	
	if(userdata->mphf_proto->func_clean(&userdata->mphf) < 0)
		return -EFAULT;
	
	if(mphf_build_start(userdata, BUILD_ONLOAD) < 0)
		return -EFAULT;
	
	// process items
	ret = 0;
	for(i=0; i<count; i++){
		request_t r_read[] = {
			{ HK(action),          DATA_INT32(ACTION_CRWD_READ)            },
			{ userdata->keyid,     DATA_VOID                               },
			{ userdata->key_from,  DATA_VOID                               },
			{ userdata->key_to,    DATA_OFFT(i)                            },
			{ HK(buffer),          DATA_RAW(buffer, userdata->buffer_size) },
			{ HK(size),            DATA_SIZET(userdata->buffer_size)       },
			hash_end
		};
		if( (ret = chain_next_query(userdata->backend_data, r_read)) < 0){
			//printf("failed call\n");
			break;
		}
		
		hash_data_find(r_read, userdata->keyid,    &key, NULL);
		if(key == NULL){
			hash_data_find(r_read, userdata->key_from, &key, NULL);
			if(key == NULL){
				//printf("failed hash\n");
				ret = -EFAULT;
				break;
			}
			
			keyid = mphf_key_str_to_key(&userdata->mphf, data_value_ptr(key), data_len(key, NULL));
		}else{
			keyid = 0;
			data_to_dt(ret, TYPE_INT64, keyid, key, NULL); 
		}
		//printf("key: %lx value: %x\n", keyid, (int)i);
		
		if( (ret = mphf_insert(userdata,
			keyid,
			i
		)) < 0){
			if(ret == -EBADF) goto redo; // bad mphf
			//printf("failed insert %x\n", ret);
			ret = -EFAULT;
			break;
		}
		
	}
	free(buffer);
	
	// if error - exit
	if(ret < 0)
		return -EFAULT;
	
	return 0;
} // }}}

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
	DT_INT64         buffer_size     = BUFFER_SIZE_DEFAULT;
	DT_INT64         count           = 0;
	DT_STRING        mphf_type_str   = NULL;
	DT_STRING        key_from_str    = "key";
	DT_STRING        key_to_str      = "offset";
	DT_STRING        keyid_str       = NULL;
	DT_STRING        build_start_str = NULL;
	DT_STRING        build_end_str   = NULL;
	DT_STRING        hash_type_str   = "jenkins";
	DT_STRING        backend_name;
	ssize_t          ret;
	mphf_userdata   *userdata = (mphf_userdata *)chain->userdata;
	
	hash_data_copy(ret, TYPE_STRING, mphf_type_str,   config, HK(type));
	hash_data_copy(ret, TYPE_STRING, key_from_str,    config, HK(key_from));
	hash_data_copy(ret, TYPE_STRING, key_to_str,      config, HK(key_to));
	hash_data_copy(ret, TYPE_STRING, keyid_str,       config, HK(keyid));
	hash_data_copy(ret, TYPE_STRING, build_start_str, config, HK(build_start));
	hash_data_copy(ret, TYPE_STRING, build_end_str,   config, HK(build_end));
	hash_data_copy(ret, TYPE_INT64,  buffer_size,     config, HK(buffer_size));
	hash_data_copy(ret, TYPE_STRING, hash_type_str,   config, HK(hash));
	hash_data_copy(ret, TYPE_STRING, backend_name,    config, HK(backend));
	if(ret != 0)
		return_error(-EINVAL, "chain 'mphf' parameter 'backend' not supplied\n");
	
	if( (userdata->mphf_proto = mphf_string_to_proto(mphf_type_str)) == NULL)
		return_error(-EINVAL, "chain 'mphf' parameter 'mphf_type' invalid or not supplied\n");
	
	if( (userdata->build_start = mphf_string_to_build_control(build_start_str)) == BUILD_INVALID)
		return_error(-EINVAL, "chain 'mphf' parameter 'build_start' invalid\n");
	if( (userdata->build_end   = mphf_string_to_build_control(build_end_str))   == BUILD_INVALID)
		return_error(-EINVAL, "chain 'mphf' parameter 'build_end' invalid\n");
	
	memset(&userdata->mphf, 0, sizeof(userdata->mphf));
	if((userdata->mphf.hash = mphf_string_to_hash_proto(hash_type_str)) == NULL)
		return_error(-EINVAL, "chain 'mphf' parameter 'hash' invalid\n");
	if( (userdata->mphf.backend = backend_acquire(backend_name)) == NULL)
		return_error(-EINVAL, "chain 'mphf' parameter 'backend' invalid\n");
	if( (userdata->mphf.config  = hash_copy(config)) == NULL)
		return_error(-ENOMEM, "chain 'mphf' insufficent memory\n");
	
	userdata->mphf.offset     = 0;
	userdata->mphf.build_data = NULL;
	userdata->key_from        = hash_string_to_key(key_from_str);
	userdata->key_to          = hash_string_to_key(key_to_str);
	userdata->keyid           = hash_string_to_key(keyid_str);
	userdata->backend_data    = chain;
	userdata->buffer_size     = buffer_size;
	
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
	uint64_t         keyid;
	mphf_userdata   *userdata = (mphf_userdata *)chain->userdata;
	
	hash_data_find(request, userdata->key_from, &key, NULL);
	if(key == NULL)
		return chain_next_query(chain, request);
	
	keyid = mphf_key_str_to_key(&userdata->mphf, data_value_ptr(key), data_value_len(key));
	
	// get new key_out
	request_t r_next[] = {
		{ userdata->keyid,     DATA_PTR_INT64(&keyid)  },
		{ HK(offset_out),      DATA_PTR_OFFT(&key_out) },
		hash_next(request)
	};
	if( (ret = chain_next_query(chain, r_next)) < 0)
		goto error;
	
	// insert key
	if( (ret = mphf_insert(userdata, keyid, key_out)) == -EBADF)
		ret = mphf_rebuild(userdata);

error:
	return ret;
} // }}}
static ssize_t mphf_backend_query(chain_t *chain, request_t *request){ // {{{
	data_t          *key;
	uint64_t         key_out, keyid;
	mphf_userdata   *userdata = (mphf_userdata *)chain->userdata;
	
	hash_data_find(request, userdata->key_from, &key, NULL);
	if(key == NULL)
		return chain_next_query(chain, request);
	
	keyid = mphf_key_str_to_key(&userdata->mphf, data_value_ptr(key), data_value_len(key));
	switch(userdata->mphf_proto->func_query(&userdata->mphf, keyid, &key_out)){
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

