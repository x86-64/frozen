#include <test.h>

START_TEST (test_real_store_nums){
	hash_t config[] = {
		{ HK(chains), DATA_HASHT(
			{ 0, DATA_HASHT(
				{ HK(name),      DATA_STRING("file")                        },
				{ HK(filename),  DATA_STRING("data_real_store_nums.dat")    },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_t  *backend = backend_new(config);
	
	off_t data_ptrs[6];
	int   data_array[] = { 100, 200, 500, 800, 888, 900 };
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(int); i++){
		request_t r_write[] = {
			{ HK(action),  DATA_INT32 (ACTION_CRWD_WRITE)  },
			{ HK(offset_out), DATA_PTR_OFFT  (&data_ptrs[i])  },
			{ HK(buffer),  DATA_PTR_INT32 (&data_array[i]) },
			hash_end
		};
		ret = backend_query(backend, r_write);
			fail_unless(ret == 4, "chain 'real_store_nums': write array failed");
	}
	
	// check
	int data_read;
	for(i=0; i < sizeof(data_array) / sizeof(int); i++){
		request_t r_read[] = {
			{ HK(action), DATA_INT32(ACTION_CRWD_READ)     },
			{ HK(offset),    DATA_OFFT(data_ptrs[i])          },
			{ HK(buffer), DATA_PTR_INT32(&data_read)       },
			hash_end
		};
		ret = backend_query(backend, r_read);
			fail_unless(ret == 4,                   "chain 'real_store_nums': read array failed");
			fail_unless(data_read == data_array[i], "chain 'real_store_nums': read array data failed");
	}
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_real_store_nums)
