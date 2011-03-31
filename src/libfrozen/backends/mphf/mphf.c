#define MPHF_C

#include <libfrozen.h>
#include <mphf.h>

#define EMODULE              10
#define BUFFER_SIZE_DEFAULT  1000
#define MAX_REBUILDS_DEFAULT 1000

typedef struct mphf_userdata {
	mphf_t               mphf;
	mphf_proto_t        *mphf_proto;
	backend_t             *backend_data;
	size_t               buffer_size;
	uintmax_t            max_rebuilds;
	
	hash_key_t           key_from;
	hash_key_t           key_to;
	hash_key_t           offset_out;
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
static ssize_t mphf_insert(mphf_userdata *userdata, uint64_t keyid, uint64_t value){ // {{{
	ssize_t   ret;
	
	// insert item
	if( (ret = userdata->mphf_proto->func_insert(
		&userdata->mphf, 
		keyid,
		value
	)) < 0)
		goto error;
	
	ret = 0;
error:
	return ret;
} // }}}
static ssize_t mphf_update(mphf_userdata *userdata, uint64_t keyid, uint64_t value){ // {{{
	ssize_t   ret;
	
	// insert item
	if( (ret = userdata->mphf_proto->func_update(
		&userdata->mphf, 
		keyid,
		value
	)) < 0)
		goto error;
	
	ret = 0;
error:
	
	return ret;
} // }}}
static ssize_t mphf_rebuild(mphf_userdata *userdata){ // {{{
	ssize_t   ret;
	off_t     i;
	off_t     count = 0;
	char     *buffer;
	data_t   *key;
	uint64_t  keyid;
	uintmax_t rebuild_num = 0;
	
	// count elements
	request_t r_count[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_COUNT) },
		{ HK(buffer), DATA_PTR_OFFT(&count)           },
		{ HK(ret),    DATA_PTR_SIZET(&ret)            },
		hash_end
	};
	if(backend_pass(userdata->backend_data, r_count) < 0 || ret < 0)
		return 0;
	
	// prepare buffer
	if( (buffer = malloc(userdata->buffer_size)) == NULL)
		return error("malloc failed");
	
redo:
	if(rebuild_num++ >= userdata->max_rebuilds)
		return error("mphf full");

	if( (ret = userdata->mphf_proto->func_clean(&userdata->mphf)) < 0)
		return ret;
	
	// process items
	for(i=0; i<count; i++){
		request_t r_read[] = {
			{ HK(action),          DATA_UINT32T(ACTION_CRWD_READ)          },
			{ userdata->keyid,     DATA_VOID                               },
			{ userdata->key_from,  DATA_VOID                               },
			{ userdata->key_to,    DATA_OFFT(i)                            },
			{ HK(buffer),          DATA_RAW(buffer, userdata->buffer_size) },
			{ HK(size),            DATA_SIZET(userdata->buffer_size)       },
			{ HK(ret),             DATA_PTR_SIZET(&ret)                    },
			hash_end
		};
		if( backend_pass(userdata->backend_data, r_read) < 0 || ret < 0){
			ret = error("failed call\n");
			break;
		}
		
		do {
			if(userdata->keyid != 0){
				hash_data_find(r_read, userdata->keyid,    &key, NULL);
				if(key != NULL){
					keyid = 0;
					data_to_dt(ret, TYPE_UINT64T, keyid, key, NULL); 
					break;
				}
			}
			
			if(userdata->key_from != 0){
				hash_data_find(r_read, userdata->key_from, &key, NULL);
				if(key != NULL){
					keyid = mphf_key_str_to_key(&userdata->mphf, data_value_ptr(key), data_len(key, NULL));
					break;
				}
			}
			ret = error("no keyid or key found in item");
			goto exit;
		}while(0);
		//printf("mphf: rebuild: key: %lx value: %x\n", keyid, (int)i);
		
		if( (ret = mphf_insert(userdata,
			keyid,
			i
		)) < 0){
			if(ret == -EBADF) goto redo; // bad mphf
			//printf("failed insert %x\n", ret);
			break;
		}
		
	}
exit:
	free(buffer);
	
	// if error - exit
	if(ret < 0)
		return ret;
	
	return 0;
} // }}}

static int mphf_init(backend_t *backend){ // {{{
	mphf_userdata *userdata = backend->userdata = calloc(1, sizeof(mphf_userdata));
	if(userdata == NULL)
		return error("calloc failed");
	
	return 0;
} // }}}
static int mphf_destroy(backend_t *backend){ // {{{
	intmax_t       ret;
	mphf_userdata *userdata = (mphf_userdata *)backend->userdata;
	
	if( (ret = userdata->mphf_proto->func_unload(&userdata->mphf)) < 0)
		return ret;
	
	hash_free(userdata->mphf.config);
	free(userdata);
	return 0;
} // }}}
static int mphf_configure(backend_t *backend, hash_t *config){ // {{{
	DT_UINT64T         buffer_size     = BUFFER_SIZE_DEFAULT;
	DT_STRING        key_from_str    = "key";
	DT_STRING        key_to_str      = "offset";
	DT_STRING        offset_out_str  = "offset_out";
	DT_STRING        keyid_str       = NULL;
	DT_STRING        mphf_type_str   = NULL;
	DT_STRING        hash_type_str   = "jenkins";
	uintmax_t        max_rebuilds    = MAX_REBUILDS_DEFAULT;
	ssize_t          ret;
	mphf_userdata   *userdata = (mphf_userdata *)backend->userdata;
	
	hash_data_copy(ret, TYPE_STRINGT, key_from_str,    config, HK(key_from));
	hash_data_copy(ret, TYPE_STRINGT, key_to_str,      config, HK(key_to));
	hash_data_copy(ret, TYPE_STRINGT, offset_out_str,  config, HK(offset_out));
	hash_data_copy(ret, TYPE_STRINGT, keyid_str,       config, HK(keyid));
	
	hash_data_copy(ret, TYPE_STRINGT, mphf_type_str,   config, HK(type));
	hash_data_copy(ret, TYPE_UINT64T,  buffer_size,     config, HK(buffer_size));
	hash_data_copy(ret, TYPE_STRINGT, hash_type_str,   config, HK(hash));
	hash_data_copy(ret, TYPE_UINTT,  max_rebuilds,    config, HK(max_rebuilds));
	
	if( (userdata->mphf_proto = mphf_string_to_proto(mphf_type_str)) == NULL)
		return error("backend mphf parameter mphf_type invalid or not supplied");
	
	memset(&userdata->mphf, 0, sizeof(userdata->mphf));
	if((userdata->mphf.hash = mphf_string_to_hash_proto(hash_type_str)) == NULL)
		return error("backend mphf parameter hash invalid");
	if( (userdata->mphf.config  = hash_copy(config)) == NULL)
		return error("backend mphf insufficent memory");
	
	userdata->key_from        = hash_string_to_key(key_from_str);
	userdata->key_to          = hash_string_to_key(key_to_str);
	userdata->offset_out      = hash_string_to_key(offset_out_str);
	userdata->keyid           = hash_string_to_key(keyid_str);
	userdata->backend_data    = backend;
	userdata->max_rebuilds    = max_rebuilds;
	userdata->buffer_size     = buffer_size;
	
	if( (ret = userdata->mphf_proto->func_load(&userdata->mphf)) < 0)
		return ret;
	
	return 0;
} // }}}

static ssize_t mphf_backend_insert(backend_t *backend, request_t *request){ // {{{
	ssize_t          ret;
	data_t          *key;
	off_t            key_out;
	uint64_t         keyid;
	mphf_userdata   *userdata = (mphf_userdata *)backend->userdata;
	
	hash_data_find(request, userdata->key_from, &key, NULL);
	if(key == NULL)
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	keyid = mphf_key_str_to_key(&userdata->mphf, data_value_ptr(key), data_value_len(key));
	
	// get new key_out
	request_t r_next[] = {
		{ userdata->keyid,      DATA_PTR_UINT64T(&keyid)  },
		{ userdata->offset_out, DATA_PTR_OFFT(&key_out)   },
		{ HK(ret),              DATA_PTR_SIZET(&ret)      },
		hash_next(request)
	};
	if( backend_pass(backend, r_next) < 0 || ret < 0)
		goto error;
	
	// insert key
	if( (ret = mphf_insert(userdata, keyid, key_out)) == -EBADF)
		ret = mphf_rebuild(userdata);
	
error:
	return ret;
} // }}}
static ssize_t mphf_backend_query(backend_t *backend, request_t *request){ // {{{
	ssize_t          ret;
	data_t          *key;
	uint64_t         key_out, key_new_out, keyid;
	mphf_userdata   *userdata = (mphf_userdata *)backend->userdata;
	
	hash_data_find(request, userdata->key_from, &key, NULL);
	if(key == NULL)
		return ( (ret = backend_pass(backend, request)) < 0) ? ret : -EEXIST;
	
	keyid = mphf_key_str_to_key(&userdata->mphf, data_value_ptr(key), data_value_len(key));
	switch(userdata->mphf_proto->func_query(&userdata->mphf, keyid, &key_out)){
		case MPHF_QUERY_NOTFOUND: return -EBADF;
		case MPHF_QUERY_FOUND:
			key_new_out = key_out;
			
			request_t r_next[] = {
				{ userdata->key_to,     DATA_PTR_OFFT(&key_out)     },
				{ userdata->offset_out, DATA_PTR_OFFT(&key_new_out) },
				hash_next(request)
			};
			if( (ret = backend_pass(backend, r_next)) < 0)
				return ret;
			
			if(key_out != key_new_out){
				ssize_t ret;
				
				// TODO ret & rebuild?
				if( (ret = mphf_update(userdata, keyid, key_new_out)) < 0)
					return ret;
			}
			
			// TODO pass captured offset_out 
	}
	return -EEXIST;
} // }}}
static ssize_t mphf_backend_queryinsert(backend_t *backend, request_t *request){ // {{{
	ssize_t ret;
	
	if((ret = mphf_backend_query(backend, request)) == -EBADF){
		return mphf_backend_insert(backend, request);
	}
	return ret;
} // }}}

backend_t mphf_proto = {
	.class          = "mphf",
	.supported_api  = API_CRWD,
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


