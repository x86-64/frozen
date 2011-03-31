
START_TEST (test_real_store_nums){
	hash_t config[] = {
                { 0, DATA_HASHT(
                        { HK(class),     DATA_STRING("file")                        },
                        { HK(filename),  DATA_STRING("data_real_store_nums.dat")    },
                        hash_end
                )},
                hash_end
	};
	
	backend_t  *backend = backend_new(config);
	        fail_unless(backend != NULL, "backend_new failed");

	off_t data_ptrs[6];
	int   data_array[] = { 100, 200, 500, 800, 888, 900 };
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(int); i++){
		request_t r_write[] = {
			{ HK(action),     DATA_UINT32T(ACTION_CRWD_CREATE)    },
			{ HK(offset_out), DATA_PTR_OFFT(&data_ptrs[i])        },
			{ HK(buffer),     DATA_PTR_UINT32T(&data_array[i])    },
			{ HK(size),       DATA_UINT32T(sizeof(data_array[i])) },
			{ HK(ret),        DATA_PTR_SIZET(&ret)                },
                        hash_end
		};
		backend_query(backend, r_write);
			fail_unless(ret == 4, "backend real_store_nums: write array failed");
	}
	
	// check
	int data_read;
	for(i=0; i < sizeof(data_array) / sizeof(int); i++){
		request_t r_read[] = {
			{ HK(action), DATA_UINT32T(ACTION_CRWD_READ)     },
			{ HK(offset), DATA_OFFT(data_ptrs[i])            },
			{ HK(buffer), DATA_PTR_UINT32T(&data_read)       },
			{ HK(ret),    DATA_PTR_SIZET(&ret)               },
			hash_end
		};
		backend_query(backend, r_read);
			fail_unless(ret == 4,                   "backend real_store_nums: read array failed");
			fail_unless(data_read == data_array[i], "backend real_store_nums: read array data failed");
	}
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_real_store_nums)
