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

typedef ssize_t  (*mphf_func_new)         (mphf_t *mphf);
typedef ssize_t  (*mphf_func_build_start) (mphf_t *mphf);
typedef ssize_t  (*mphf_func_build_end)   (mphf_t *mphf);
typedef ssize_t  (*mphf_func_insert)      (mphf_t *mphf, char *key, size_t key_len, uint64_t  value);
typedef ssize_t  (*mphf_func_query)       (mphf_t *mphf, char *key, size_t key_len, uint64_t *value);
typedef ssize_t  (*mphf_func_delete)      (mphf_t *mphf, char *key, size_t key_len);

typedef uint32_t (*mphf_hash_hash32)      (uint32_t seed, char *k, size_t keylen, uint32_t hashes[], size_t nhashes);

struct mphf_t {
	config_t                *config;
	backend_t               *backend;
	void                    *build_data;
	uint64_t                 offset;
	
	char                     data[112];
};

struct mphf_proto_t {
	mphf_func_new            func_new;
	mphf_func_build_start    func_build_start;
	mphf_func_build_end      func_build_end;
	
	mphf_func_insert         func_insert;
	mphf_func_query          func_query;
	mphf_func_delete         func_delete;
};

struct mphf_hash_proto_t {
	mphf_hash_hash32         func_hash32;
};

ssize_t         mphf_store_new   (backend_t *backend, uint64_t *offset, uint64_t size);
ssize_t         mphf_store_read  (backend_t *backend, uint64_t  offset, void *buffer, size_t buffer_size);
ssize_t         mphf_store_write (backend_t *backend, uint64_t  offset, void *buffer, size_t buffer_size);
ssize_t         mphf_store_fill  (backend_t *backend, uint64_t  offset, void *buffer, size_t buffer_size, size_t fill_size);

mphf_hash_proto_t *  mphf_string_to_hash_proto (char *string);
uint32_t             mphf_hash32               (mphf_hash_types type, uint32_t seed, void *key, size_t key_size, uint32_t hashes[], size_t nhashes);

#ifdef MPHF_C
#include <backends/mphf/mphf_chm_imp.h>
#include <backends/mphf/mphf_bdz_imp.h>

static mphf_proto_t mphf_protos[] = {
	[MPHF_TYPE_CHM_IMP] = {
		.func_new           = mphf_chm_imp_new,
		.func_build_start   = mphf_chm_imp_build_start,
		.func_build_end     = mphf_chm_imp_build_end,
		.func_insert        = mphf_chm_imp_insert,
		.func_query         = mphf_chm_imp_query,
		.func_delete        = mphf_chm_imp_delete
	}
};
//static size_t mphf_protos_size = sizeof(mphf_protos) / sizeof(mphf_protos[0]);

#include <backends/mphf/hash_jenkins.h>
static mphf_hash_proto_t mphf_hash_protos[] = {
	[MPHF_HASH_JENKINS32] = {
		.func_hash32  = jenkins32_hash
	}
};
static size_t mphf_hash_protos_size = sizeof(mphf_hash_protos) / sizeof(mphf_hash_protos[0]);

#endif
#endif
