typedef struct structs_test {
	int   key1;
	char *key2;
} structs_test;

START_TEST(test_backend_structs){
	backend_t  *backend;
	
	hash_t  settings[] = {
		{ HK(chains), DATA_HASHT(
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("file")                        },
				{ HK(filename),    DATA_STRING("data_backend_structs.dat")    },
				hash_end
			)},
			{ 0, DATA_HASHT(
				{ HK(name),        DATA_STRING("structs")                     },
				{ HK(structure),   DATA_HASHT(
					{ HK(key1),  DATA_INT32(0)   },
					{ HK(key2),  DATA_STRING("") },
					hash_end
				)},
				{ HK(size),        DATA_STRING("size")                        },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	/* create backend */
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	off_t  data_ptrs[6];
	structs_test  data_array[] = {
		{ 1, "test1" },
		{ 2, "test22" },
		{ 3, "test333" },
		{ 4, "test4444" },
		{ 5, "test55555" },
		{ 6, "test666666" }
	};
	
	// write array to file
	int        i;
	ssize_t    ret;
	char       test[100];
	DT_INT32   test1;
	DT_STRING  test2;
	
	for(i=0; i < sizeof(data_array) / sizeof(data_array[0]); i++){
		request_t r_write[] = {
			{ HK(action),     DATA_INT32(ACTION_CRWD_CREATE)                          },
			{ HK(offset_out), DATA_PTR_OFFT(&data_ptrs[i])                            },
			{ HK(buffer),     DATA_RAW(test, 100)                                     },
			
			{ HK(key1),       DATA_PTR_INT32      (&data_array[i].key1)               },
			{ HK(key2),       DATA_PTR_STRING_AUTO( data_array[i].key2)               },
			hash_end
		};
		ret = backend_query(backend, r_write);
			fail_unless(ret > 0, "write array failed");
	}
	
	// check
	for(i=0; i < sizeof(data_array) / sizeof(data_array[0]); i++){
		request_t r_read[] = {
			{ HK(action), DATA_INT32(ACTION_CRWD_READ)      },
			{ HK(offset), DATA_OFFT(data_ptrs[i])           },
			{ HK(buffer), DATA_RAW(test, 100)               },
			
			{ HK(key1),   DATA_INT32(0)                     },
			{ HK(key2),   DATA_STRING("")                   },
			hash_end
		};
		ret = backend_query(backend, r_read);
			fail_unless(ret > 0,                               "read array failed");
		
		hash_data_copy(ret, TYPE_INT32,  test1, r_read, HK(key1));
			fail_unless(ret == 0 && test1 == data_array[i].key1,           "data 1 failed\n");
		hash_data_copy(ret, TYPE_STRING, test2, r_read, HK(key2));
			fail_unless(ret == 0 && strcmp(test2,data_array[i].key2) == 0, "data 2 failed\n");
	}
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_structs)
