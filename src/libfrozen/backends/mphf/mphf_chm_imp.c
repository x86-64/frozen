#include <libfrozen.h>
#include <backends/mphf/mphf.h>

// this is improved chm algo:
// 	- ordinary graph replaced with 3-uniform hypergraph, so c=1.23 now instead of c=2.09
// 	- graph generation and acyclic check removed as unnessesary: while filling g() we can mark
// 	  occupied items and find out cycles by occupied vertexes of new edge
// 	- absense of graph lead to easy key insert till cycle found

typedef enum chm_imp_stores {
	CHM_IMP_STORE_PARAMS = 0,
	CHM_IMP_STORE_GO     = 1,
	CHM_IMP_STORE_G      = 2
} chm_imp_stores;

typedef struct chm_imp_t {
	uint64_t          nelements;
	uint32_t          value_bits;
} chm_imp_t;

ssize_t mphf_chm_imp_new    (mphf_t *mphf, uint64_t nelements, uint32_t value_bits){
	chm_imp_t  data;
	
	data.nelements  = nelements;
	data.value_bits = value_bits;
	
	if(mphf_store_new(mphf, CHM_IMP_STORE_PARAMS, sizeof(chm_imp_t) ) < 0)                          return -EFAULT;
	if(mphf_store_new(mphf, CHM_IMP_STORE_GO,     ( (nelements >> 3) + 1) ) < 0)                    return -EFAULT;
	if(mphf_store_new(mphf, CHM_IMP_STORE_G,      ( ((value_bits >> 3) + 1) * nelements + 1) ) < 0) return -EFAULT;
	
	mphf_store_write (mphf, CHM_IMP_STORE_PARAMS, 0, &data, sizeof(chm_imp_t));
	
	return 0;
}

ssize_t mphf_chm_imp_insert (mphf_t *mphf, char *key, size_t key_len, uint64_t  value){
	chm_imp_t  data;
	
	mphf_store_read(mphf, CHM_IMP_STORE_PARAMS, 0, &data, sizeof(chm_imp_t));
	
	printf("mphf: insert %.*s\n", key_len, key);

	return 0;
}

ssize_t mphf_chm_imp_query  (mphf_t *mphf, char *key, size_t key_len, uint64_t *value){
	chm_imp_t  data;
	
	mphf_store_read(mphf, CHM_IMP_STORE_PARAMS, 0, &data, sizeof(chm_imp_t));
	
	printf("mphf: query %.*s\n", key_len, key);
	return 0;
}

uint32_t mphf_chm_imp_nstores  (uint64_t nelements, uint32_t value_bits){
	(void)nelements; (void)value_bits;
	
	return 3;
}

