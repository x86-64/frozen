#ifndef MPHF_H
#define MPHF_H

#define MPHF_QUERY_FOUND    1
#define MPHF_QUERY_NOTFOUND 0

typedef enum mphf_types {
	MPHF_TYPE_INVALID = 0,
	MPHF_TYPE_CHM_IMP = 1,
	MPHF_TYPE_BDZ_IMP = 2
} mphf_types;

typedef struct mphf_store_t {
	uint64_t            offset;
} mphf_store_t;

typedef struct mphf_params_t {
	uint32_t            type;      // enum mphf_types
	uint32_t            nstores;
} mphf_params_t;

typedef struct mphf_t {
	mphf_params_t       params;
	
	mphf_store_t       *stores;
	backend_t          *backend;
} mphf_t;

ssize_t         mphf_store_new   (mphf_t *mphf, uint32_t id, uint64_t size);
ssize_t         mphf_store_read  (mphf_t *mphf, uint32_t id, uint64_t offset, void *buffer, size_t buffer_size);
ssize_t         mphf_store_write (mphf_t *mphf, uint32_t id, uint64_t offset, void *buffer, size_t buffer_size);

ssize_t         mphf_new         (mphf_t *mphf, backend_t *backend, uint64_t *offset, mphf_types type, uint64_t nelements, uint32_t value_bits);
ssize_t         mphf_load        (mphf_t *mphf, backend_t *backend, uint64_t  offset);
ssize_t         mphf_insert      (mphf_t *mphf, char *key, size_t keylen, uint64_t  value);
ssize_t         mphf_query       (mphf_t *mphf, char *key, size_t keylen, uint64_t *value);
void            mphf_free        (mphf_t *mphf);

typedef enum mphf_hash_types {
	MPHF_HASH_JENKINS32
} mphf_hash_types;

uint32_t        mphf_hash32      (mphf_hash_types type, uint32_t seed, void *key, size_t key_size, uint32_t hashes[], size_t nhashes);

#endif
