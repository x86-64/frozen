/*
#include <libfrozen.h>
#include <backends/mphf/mphf.h>

// NOT WORK

typedef enum bdz_imp_stores {
	BDZ_IMP_STORE_PARAMS = 0,
	BDZ_IMP_STORE_G      = 1
} bdz_imp_stores;

typedef struct bdz_imp_t {
	uint64_t          r;
	uint32_t          value_bytes;
	uint32_t          hash_type;
	uint32_t          hash1;
	//uint32_t          hash2;
	//uint32_t          hash3;
} bdz_imp_t;

typedef struct gval {
	uint64_t  offset;
	uint64_t  value;
	char      occupied;
} gval;

#define N_INITIAL_DEFAULT    256
#define VALUE_BITS_DEFAULT   31

ssize_t mphf_bdz_imp_new    (mphf_t *mphf, config_t *config){
	bdz_imp_t  data;
	uint64_t   g_size;
	uint32_t   value_bytes;
	ssize_t    ret;
	DT_INT64   nelements      = N_INITIAL_DEFAULT;
	DT_INT32   value_bits     = VALUE_BITS_DEFAULT;
	
	hash_copy_data(ret, TYPE_INT64,  nelements,     config, HK(n_initial));
	hash_copy_data(ret, TYPE_INT32,  value_bits,    config, HK(value_bits));
	
	srandom(time(NULL));
	
	data.hash_type  = MPHF_HASH_JENKINS32;
	data.hash1      = 4;
	
	value_bytes = MAX(1, ((value_bits + 1) >> 3));
	
	// r = (1.23 * nelements) / 3
	g_size = nelements;
	g_size = (__MAX(uint64_t) / 123 > g_size) ? (g_size * 123) / 100 : (g_size / 100) * 123;
	g_size = (g_size / 3);
	if((g_size % 2) == 0) g_size++;
	
	data.r             = g_size;
	data.value_bytes   = value_bytes;
	
	if(__MAX(uint64_t) / 3 <= g_size)
		return -EINVAL;
	g_size *= 3;
	
	//printf("mphf new nelem: %lld hashelem: %lld g_size: %lld\n", nelements, data.r, g_size);
	
	if(__MAX(uint64_t) / value_bytes <= g_size)
		return -EINVAL;
	g_size *= value_bytes;
	
	if(mphf_store_new(mphf, BDZ_IMP_STORE_PARAMS, sizeof(bdz_imp_t) ) < 0) return -EFAULT;
	if(mphf_store_new(mphf, BDZ_IMP_STORE_G,      g_size            ) < 0) return -EFAULT;
	
	mphf_store_write (mphf, BDZ_IMP_STORE_PARAMS, 0, &data, sizeof(bdz_imp_t));
	
	return 0;
}

static ssize_t  mphf_bdz_imp_getg(bdz_imp_t *data, mphf_t *mphf, char *key, size_t key_len, gval *array){
	uint32_t   hash[3];
	uint64_t   occupied_mask;
	uint64_t   tmp;
	
	// TODO 64 bit version
	mphf_hash32(data->hash_type, data->hash1, key, key_len, (uint32_t *)&hash, 3); 
	
	array[0].offset = (hash[0] % data->r                 ) * data->value_bytes;
	array[1].offset = (hash[1] % data->r + (data->r     )) * data->value_bytes;
	array[2].offset = (hash[2] % data->r + (data->r << 1)) * data->value_bytes;
	
	occupied_mask = ((uint64_t)1 << ((data->value_bytes << 3) - 1));
	
	tmp = 0;
	mphf_store_read(mphf, BDZ_IMP_STORE_G, array[0].offset, &tmp, data->value_bytes);
	array[0].occupied = ( (tmp & occupied_mask) == 0) ? 0 : 1;
	array[0].value    = tmp & ~occupied_mask;
	
	tmp = 0;
	mphf_store_read(mphf, BDZ_IMP_STORE_G, array[1].offset, &tmp, data->value_bytes);
	array[1].occupied = ( (tmp & occupied_mask) == 0) ? 0 : 1;
	array[1].value    = tmp & ~occupied_mask;
	
	tmp = 0;
	mphf_store_read(mphf, BDZ_IMP_STORE_G, array[2].offset, &tmp, data->value_bytes);
	array[2].occupied = ( (tmp & occupied_mask) == 0) ? 0 : 1;
	array[2].value    = tmp & ~occupied_mask;
	
	//printf("0: %llx 1: %llx 2: %llx - %d %d %d\n",
	//	array[0].offset, array[1].offset, array[2].offset,
	//	array[0].occupied, array[1].occupied, array[2].occupied
	//);
	
	return 0;
}

ssize_t mphf_bdz_imp_insert (mphf_t *mphf, char *key, size_t key_len, uint64_t  value){
	uint64_t  *g_free;
	uint64_t   occupied_mask;
	gval       array[3];
	bdz_imp_t  data;
	
	mphf_store_read   (mphf, BDZ_IMP_STORE_PARAMS, 0, &data, sizeof(bdz_imp_t));
	mphf_bdz_imp_getg (&data, mphf, key, key_len, (gval *)&array);
	
	      if(array[0].occupied == 0){ g_free = &array[0].value;
	}else if(array[1].occupied == 0){ g_free = &array[1].value;
	}else if(array[2].occupied == 0){ g_free = &array[2].value;
	}else{
		// call rebuild
		//return_error(-EFAULT, "mphf g_free\n");
		return -EFAULT;
	}
	
	*g_free = value - (array[0].value + array[1].value + array[2].value);
	
	occupied_mask = ((uint64_t)1 << ((data.value_bytes << 3) - 1));
	
	//printf("mphf: insert %.*s %.8llx\n", key_len, key, value);
	
	if(array[0].occupied == 0){
		array[0].value |= occupied_mask;
		mphf_store_write(mphf, BDZ_IMP_STORE_G, array[0].offset, &array[0].value, data.value_bytes);
	}
	if(array[1].occupied == 0){
		array[1].value |= occupied_mask;
		mphf_store_write(mphf, BDZ_IMP_STORE_G, array[1].offset, &array[1].value, data.value_bytes);
	}
	if(array[2].occupied == 0){
		array[2].value |= occupied_mask;
		mphf_store_write(mphf, BDZ_IMP_STORE_G, array[2].offset, &array[2].value, data.value_bytes);
	}
	return 0;
}

ssize_t mphf_bdz_imp_query  (mphf_t *mphf, char *key, size_t key_len, uint64_t *value){
	gval       array[3];
	bdz_imp_t  data;
	
	mphf_store_read   (mphf, BDZ_IMP_STORE_PARAMS, 0, &data, sizeof(bdz_imp_t));
	mphf_bdz_imp_getg (&data, mphf, key, key_len, (gval *)&array);
	
	if(array[0].occupied == 0 || array[1].occupied == 0 || array[2].occupied == 0)
		return MPHF_QUERY_NOTFOUND;
	
	*value = (array[0].value + array[1].value + array[2].value);
	
	//printf("mphf: query %.*s value: %.8llx\n", key_len, key, *value);
	return MPHF_QUERY_FOUND;
}

uint32_t mphf_bdz_imp_nstores  (config_t *config){
	(void)config;

	return 2;
}
*/
