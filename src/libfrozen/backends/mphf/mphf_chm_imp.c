#include <libfrozen.h>
#include <backends/mphf/mphf.h>

// this is improved chm algo:
// 	- ordinary graph replaced with 3-uniform hypergraph, so c=1.23 now instead of c=2.09
// 	- graph generation and acyclic check removed as unnessesary: while filling g() we can mark
// 	  occupied items and find out cycles by occupied vertexes of new edge
// 	- absense of graph lead to easy key insert till cycle found
// 	- even hash collisions which map one item in only two different vertexes is not problem -
// 	  we only need one empty vertex

typedef enum chm_imp_stores {
	CHM_IMP_STORE_PARAMS = 0,
	CHM_IMP_STORE_G      = 1
} chm_imp_stores;

typedef struct chm_imp_t {
	uint64_t          hash_elements;
	uint32_t          value_bytes;
	uint32_t          hash_type;
	uint32_t          hash1;
	uint32_t          hash2;
	uint32_t          hash3;
} chm_imp_t;

typedef struct gval {
	uint64_t  offset;
	char      occupied;
	uint64_t  value;
} gval;

ssize_t mphf_chm_imp_new    (mphf_t *mphf, uint64_t nelements, uint32_t value_bits){
	chm_imp_t  data;
	uint64_t   g_size;
	uint32_t   value_bytes;
	
	srandom(time(NULL));
	
	data.hash_type  = MPHF_HASH_JENKINS32;
	data.hash1      = random();
	data.hash2      = random();
	data.hash3      = random();
	
	value_bytes = ((value_bits + 1) >> 3);
	if(value_bytes == 0) value_bytes = 1;
	
	g_size      = (__MAX(uint64_t) / 23 > nelements) ?
		(nelements * 23) / 100 :
		(nelements / 100) * 23;
	
	if(__MAX(uint64_t) - g_size <= nelements)
		return -EINVAL;
	g_size += nelements;
	
	data.hash_elements = g_size;
	data.value_bytes   = value_bytes;
	
	if(__MAX(uint64_t) / value_bytes <= g_size)
		return -EINVAL;
	g_size *= value_bytes;
	
	if(mphf_store_new(mphf, CHM_IMP_STORE_PARAMS, sizeof(chm_imp_t) ) < 0) return -EFAULT;
	if(mphf_store_new(mphf, CHM_IMP_STORE_G,      g_size            ) < 0) return -EFAULT;
	
	mphf_store_write (mphf, CHM_IMP_STORE_PARAMS, 0, &data, sizeof(chm_imp_t));
	
	return 0;
}

static ssize_t  mphf_chm_imp_getg(chm_imp_t *data, mphf_t *mphf, char *key, size_t key_len, gval *array){
	uint32_t   hash1, hash2, hash3;
	uint64_t   occupied_mask;
	uint64_t   tmp = 0;
	
	// TODO 64 bit version
	hash1 = mphf_hash32(data->hash_type, data->hash1, key, key_len); 
	hash2 = mphf_hash32(data->hash_type, data->hash2, key, key_len); 
	hash3 = mphf_hash32(data->hash_type, data->hash3, key, key_len); 
	
	hash1 = hash1 % data->hash_elements;
	hash2 = hash2 % data->hash_elements;
	hash3 = hash3 % data->hash_elements;
	
	array[0].offset = hash1 * data->value_bytes;
	array[1].offset = hash2 * data->value_bytes;
	array[2].offset = hash3 * data->value_bytes;
	
	occupied_mask = ((uint64_t)1 << ((data->value_bytes << 3) - 1));
	
	mphf_store_read(mphf, CHM_IMP_STORE_G, array[0].offset, &tmp, data->value_bytes);
	array[0].occupied = ( (tmp & occupied_mask) == 0) ? 0 : 1;
	array[0].value    = tmp & ~occupied_mask; //tmp - ((array[0].occupied == 1) ? occupied_mask : 0);
	
	mphf_store_read(mphf, CHM_IMP_STORE_G, array[1].offset, &tmp, data->value_bytes);
	array[1].occupied = ( (tmp & occupied_mask) == 0) ? 0 : 1;
	array[1].value    = tmp & ~occupied_mask; //tmp - ((array[0].occupied == 1) ? occupied_mask : 0);
	
	mphf_store_read(mphf, CHM_IMP_STORE_G, array[2].offset, &tmp, data->value_bytes);
	array[2].occupied = ( (tmp & occupied_mask) == 0) ? 0 : 1;
	array[2].value    = tmp & ~occupied_mask; //tmp - ((array[0].occupied == 1) ? occupied_mask : 0);
	
	return 0;
}

ssize_t mphf_chm_imp_insert (mphf_t *mphf, char *key, size_t key_len, uint64_t  value){
	uint64_t  *g_free;
	uint64_t   occupied_mask;
	gval       array[3];
	chm_imp_t  data;
	
	mphf_store_read   (mphf, CHM_IMP_STORE_PARAMS, 0, &data, sizeof(chm_imp_t));
	mphf_chm_imp_getg (&data, mphf, key, key_len, (gval *)&array);
	
	      if(array[0].occupied == 0){ g_free = &array[0].value;
	}else if(array[1].occupied == 0){ g_free = &array[1].value;
	}else if(array[2].occupied == 0){ g_free = &array[2].value;
	}else{
		// call rebuild
		return_error(-EFAULT, "mphf g_free\n");
	}
	
	*g_free = value - (array[0].value + array[1].value + array[2].value);
	
	occupied_mask = ((uint64_t)1 << ((data.value_bytes << 3) - 1));
	
	array[0].value |= occupied_mask;
	array[1].value |= occupied_mask;
	array[2].value |= occupied_mask;
	
	//printf("mphf: insert %.*s %.8llx\n", key_len, key, value);
	
	mphf_store_write(mphf, CHM_IMP_STORE_G, array[0].offset, &array[0].value, data.value_bytes);
	mphf_store_write(mphf, CHM_IMP_STORE_G, array[1].offset, &array[1].value, data.value_bytes);
	mphf_store_write(mphf, CHM_IMP_STORE_G, array[2].offset, &array[2].value, data.value_bytes);
	return 0;
}

ssize_t mphf_chm_imp_query  (mphf_t *mphf, char *key, size_t key_len, uint64_t *value){
	gval       array[3];
	chm_imp_t  data;
	
	mphf_store_read   (mphf, CHM_IMP_STORE_PARAMS, 0, &data, sizeof(chm_imp_t));
	mphf_chm_imp_getg (&data, mphf, key, key_len, (gval *)&array);
	
	if(array[0].occupied == 0 || array[1].occupied == 0 || array[2].occupied == 0)
		return MPHF_QUERY_NOTFOUND;
	
	*value = (array[0].value + array[1].value + array[2].value);
	
	//printf("mphf: query %.*s value: %.8llx\n", key_len, key, *value);
	return MPHF_QUERY_FOUND;
}

uint32_t mphf_chm_imp_nstores  (uint64_t nelements, uint32_t value_bits){
	(void)nelements; (void)value_bits;
	
	return 2;
}

