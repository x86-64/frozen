#ifndef MPHF_H
#define MPHF_H

#define MPHF_QUERY_FOUND    1
#define MPHF_QUERY_NOTFOUND 0

typedef enum mphf_types {
	MPHF_TYPE_INVALID = 0,
	MPHF_TYPE_CHM_IMP = 1,
	MPHF_TYPE_BDZ_IMP = 2
} mphf_types;

typedef enum mphf_hash_types {
	MPHF_HASH_JENKINS32 = 0
} mphf_hash_types;


typedef struct mphf_t             mphf_t;
typedef struct mphf_proto_t       mphf_proto_t;
typedef struct mphf_hash_proto_t  mphf_hash_proto_t;

typedef ssize_t  (*mphf_func_load)        (mphf_t *mphf);
typedef ssize_t  (*mphf_func_unload)      (mphf_t *mphf);
typedef ssize_t  (*mphf_func_rebuild)     (mphf_t *mphf, uint64_t e_nelements);
typedef ssize_t  (*mphf_func_fork)        (mphf_t *mphf, request_t *request);
typedef ssize_t  (*mphf_func_destroy)     (mphf_t *mphf);
typedef ssize_t  (*mphf_func_insert)      (mphf_t *mphf, uint64_t key, uint64_t  value);
typedef ssize_t  (*mphf_func_update)      (mphf_t *mphf, uint64_t key, uint64_t  value);
typedef ssize_t  (*mphf_func_query)       (mphf_t *mphf, uint64_t key, uint64_t *value);
typedef ssize_t  (*mphf_func_delete)      (mphf_t *mphf, uint64_t key);

typedef uint32_t (*mphf_hash_hash32)      (uint32_t seed, char *k, size_t keylen, uint32_t hashes[], size_t nhashes);

struct mphf_t {
	config_t                *config;
	void                    *build_data;
	mphf_hash_proto_t       *hash;
	
	char                     data[112];
};

struct mphf_proto_t {
	mphf_func_load           func_load;
	mphf_func_unload         func_unload;
	mphf_func_fork           func_fork;
	mphf_func_rebuild        func_rebuild;
	mphf_func_destroy        func_destory;
	
	mphf_func_insert         func_insert;
	mphf_func_update         func_update;
	mphf_func_query          func_query;
	mphf_func_delete         func_delete;
};

struct mphf_hash_proto_t {
	mphf_hash_hash32         func_hash32;
};

mphf_hash_proto_t *  mphf_string_to_hash_proto (char *string);
uint32_t             mphf_hash32               (mphf_hash_types type, uint32_t seed, void *key, size_t key_size, uint32_t hashes[], size_t nhashes);

#ifdef MPHF_C
#include <mphf_chm_imp.h>
#include <mphf_bdz_imp.h>

static mphf_proto_t mphf_protos[] = {
	[MPHF_TYPE_CHM_IMP] = {
		.func_load          = mphf_chm_imp_load,
		.func_unload        = mphf_chm_imp_unload,
		.func_fork          = mphf_chm_imp_fork,
		.func_rebuild       = mphf_chm_imp_rebuild,
		.func_destory       = mphf_chm_imp_destroy,
		.func_insert        = mphf_chm_imp_insert,
		.func_update        = mphf_chm_imp_update,
		.func_query         = mphf_chm_imp_query,
		.func_delete        = mphf_chm_imp_delete
	}
};
//static size_t mphf_protos_size = sizeof(mphf_protos) / sizeof(mphf_protos[0]);

#include <hash_jenkins.h>
static mphf_hash_proto_t mphf_hash_protos[] = {
	[MPHF_HASH_JENKINS32] = {
		.func_hash32  = jenkins32_hash
	}
};
static size_t mphf_hash_protos_size = sizeof(mphf_hash_protos) / sizeof(mphf_hash_protos[0]);

#endif
#endif
