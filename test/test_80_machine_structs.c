typedef struct structs_test {
	int   key1;
	char *key2;
} structs_test;

START_TEST(test_machine_structs){
	machine_t  *machine;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("file")                        },
                        { HK(filename),    DATA_STRING("data_machine_structs.dat")    },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),       DATA_STRING("structs")                     },
                        { HK(structure),   DATA_HASHT(
                                { HK(key1),  DATA_HASHT(
					{ HK(default), DATA_UINT32T(0)                },
					hash_end
				)},
                                { HK(key2),  DATA_HASHT(
					{ HK(default), DATA_STRING("")                },
					hash_end
				)},
                                hash_end
                        )},
                        { HK(size),        DATA_HASHKEYT(HK(size))                    },
                        hash_end
                )},
                hash_end
	};
	
	/* create machine */
	machine = machine_new(settings);
		fail_unless(machine != NULL, "machine creation failed");
	
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
	char       test[100] = { 0 };
	uint32_t   test1;
	char       test2[100];
	
	for(i=0; i < sizeof(data_array) / sizeof(data_array[0]); i++){
		request_t r_write[] = {
			{ HK(action),     DATA_UINT32T(ACTION_CREATE)                              },
			{ HK(offset_out), DATA_PTR_OFFT(&data_ptrs[i])                             },
			{ HK(buffer),     DATA_RAW(test, 100)                                      },
			
			{ HK(key1),       DATA_PTR_UINT32T      (&data_array[i].key1)                  },
			{ HK(key2),       DATA_RAW(data_array[i].key2, strlen(data_array[i].key2) + 1) },
			{ HK(ret),        DATA_PTR_SIZET(&ret)                                         },
                        hash_end
		};
		machine_query(machine, r_write);
			fail_unless(ret == 0, "write array failed");
	}
	
	// check
	for(i=0; i < sizeof(data_array) / sizeof(data_array[0]); i++){
		request_t r_read[] = {
			{ HK(action), DATA_UINT32T(ACTION_READ)           },
			{ HK(offset), DATA_OFFT(data_ptrs[i])             },
			{ HK(buffer), DATA_RAW(test, 100)                 },
			
			{ HK(key1),   DATA_PTR_UINT32T(&test1)            },
			{ HK(key2),   DATA_RAW(test2, 100)                },
			{ HK(ret),    DATA_PTR_SIZET(&ret)                },
			hash_end
		};
		machine_query(machine, r_read);
			fail_unless(ret == 0,                                                            "read array failed");
			fail_unless(test1 == data_array[i].key1,                                         "data 1 failed\n");
			fail_unless(strncmp(test2, data_array[i].key2, strlen(data_array[i].key2)) == 0, "data 2 failed\n");
	}
	machine_destroy(machine);
}
END_TEST
REGISTER_TEST(core, test_machine_structs)
