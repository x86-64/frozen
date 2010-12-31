#include <libfrozen.h>
#include <backends/mphf/mphf.h>

// this is improved chm algo:
// 	- ordinary graph replaced with 3-uniform hypergraph, so c=1.23 now instead of c=2.09
// 	- graph generation and acyclic check removed as unnessesary: while filling g() we can mark
// 	  occupied items and find out cycles by occupied vertexes of new edge
// 	- absense of graph leading to easy key insert till cycle found

typedef struct chm_imp_t {
	uint64_t      nelements;
	size_t        bytes_per_value;
	
} chm_imp_t;

void mphf_chm_imp_new    (void **userdata, uint64_t nelements, size_t value_bits){
	chm_imp_t *data = *userdata = malloc(sizeof(chm_imp_t));
	data->nelements       = nelements;
	data->bytes_per_value = value_bits >> 3;
	
	
}

void mphf_chm_imp_insert (void *userdata, char *key, size_t key_len, uint64_t  value){
	
}

void mphf_chm_imp_query  (void *userdata, char *key, size_t key_len, uint64_t *value){
	
}

