#include <test.h>

START_TEST (test_real_store_nums){
	hash_t config[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                        },
				{ "filename",  DATA_STRING("data_real_store_nums.dat")    },
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
			{ "action",  DATA_INT32 (ACTION_CRWD_WRITE)  },
			{ "key_out", DATA_PTR_OFFT  (&data_ptrs[i])  },
			{ "buffer",  DATA_PTR_INT32 (&data_array[i]) },
			hash_end
		};
		ret = backend_query(backend, r_write);
			fail_unless(ret == 4, "chain 'real_store_nums': write array failed");
	}
	
	// check
	int data_read;
	for(i=0; i < sizeof(data_array) / sizeof(int); i++){
		request_t r_read[] = {
			{ "action", DATA_INT32(ACTION_CRWD_READ)     },
			{ "key",    DATA_OFFT(data_ptrs[i])          },
			{ "buffer", DATA_PTR_INT32(&data_read)       },
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

START_TEST (test_real_store_strings){
	hash_t config[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                        },
				{ "filename",  DATA_STRING("data_real_store_strings.dat") },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_t  *backend = backend_new(config);
	
	off_t  data_ptrs[6];
	char  *data_array[] = {
		"http://google.ru/",
		"http://yandex.ru/",
		"http://bing.com/",
		"http://rambler.ru/",
		"http://aport.ru/",
		"http://hell.com/"
	};
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_write[] = {
			{ "action",  DATA_INT32 (ACTION_CRWD_WRITE)                           },
			{ "key_out", DATA_PTR_OFFT   (&data_ptrs[i])                          },
			{ "buffer",  DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1) },
			hash_end
		};
		ret = backend_query(backend, r_write);
			fail_unless(ret > 0, "chain 'real_store_strings': write array failed");
	}
	
	// check
	char data_read[1024];
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_read[] = {
			{ "action", DATA_INT32(ACTION_CRWD_READ)      },
			{ "key",    DATA_OFFT(data_ptrs[i])           },
			{ "buffer", DATA_PTR_STRING(&data_read, 1024) },
			hash_end
		};
		ret = backend_query(backend, r_read);
			fail_unless(ret > 0,                               "chain 'real_store_strings': read array failed");
			fail_unless(strcmp(data_read, data_array[i]) == 0, "chain 'real_store_strings': read array data failed");
	}
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_real_store_strings)

START_TEST (test_real_store_idx_strings){
	hash_t c_data[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",      DATA_STRING("file")                           },
				{ "filename",  DATA_STRING("data_real_store_dat_string.dat") },
				hash_end
			)},
			hash_end
		)},
		{ "name", DATA_STRING("b_data") },
		hash_end
	};
	backend_t  *b_data = backend_new(c_data);
	
	hash_t c_idx[] = {
		{ NULL, DATA_HASHT(
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("file")                               },
				{ "filename",     DATA_STRING("data_real_store_idx_string.dat")     },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("rewrite")                            },
				{ "script",       DATA_STRING(
					"request_t rq_data;                                                  "
					
					"if(!data_cmp(request['action'], read)){                             "
					"   data_arith((string)'*', request['key'], (off_t)'8');             "
					"                                                                    "
					"   rq_data['buffer'] = request['buffer'];                           "
					"                                                                    "
					"   request['buffer'] = data_alloca((string)'off_t', (size_t)'8');   "
					"   pass(request);                                                   "
					"                                                                    "
					"   rq_data['action'] = read;                                        "
					"   rq_data['key']    = request['buffer'];                           " 
					"   ret = backend((string)'b_data', rq_data);                        "
					"};                                                                  "
					
					"if(!data_cmp(request['action'], write)){                            "
					"   rq_data['action']  = write;                                      "
					"   rq_data['buffer']  = request['buffer'];                          "
					"   rq_data['key_out'] = data_alloca((string)'memory', (size_t)'8'); "
					"   ret = backend((string)'b_data', rq_data);                        "
					"                                                                    "
					"   request['buffer'] = rq_data['key_out'];                          "
		 			"   data_arith((string)'*', request['key'], (off_t)'8');             "
					"   pass(request);                                                   "
					"   data_arith((string)'/', request['key_out'], (off_t)'8');         "
					"};                                                                  "
					
					"if(!data_cmp(request['action'], delete)){                           "
					"   data_arith((string)'*', request['key'], (off_t)'8');             "
					"   ret = pass(request);                                             "
					"};                                                                  "
					
					"if(!data_cmp(request['action'], move)){                             "
					"   data_arith((string)'*', request['key_to'],   (off_t)'8');        "
					"   data_arith((string)'*', request['key_from'], (off_t)'8');        "
					"   ret = pass(request);                                             "
					"};                                                                  " 
					
					"if(!data_cmp(request['action'], count)){                            "
					"   ret = pass(request);                                             "
					"   data_arith((string)'/', request['buffer'], (off_t)'8');          "
					"};                                                                  "
				)},
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("list")                               },
				hash_end
			)},
			{ NULL, DATA_HASHT(
				{ "name",         DATA_STRING("insert-sort")                        },
				{ "engine",       DATA_STRING("binsearch")                          },
				hash_end
			)},
			hash_end
		)},
		hash_end
	};
	
	backend_t  *b_idx = backend_new(c_idx);
	
	off_t  data_ptrs[6];
	char  *data_array[] = {
		"http://google.ru/",
		"http://yandex.ru/",
		"http://bing.com/",
		"http://rambler.ru/",
		"http://aport.ru/",
		"http://hell.com/"
	};
	
	// write array to file
	int      i;
	ssize_t  ret;
	
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_write[] = {
			{ "action",  DATA_INT32 (ACTION_CRWD_WRITE)                           },
			{ "key_out", DATA_PTR_OFFT   (&data_ptrs[i])                          },
			{ "buffer",  DATA_PTR_STRING (data_array[i], strlen(data_array[i])+1) },
			hash_end
		};
		ret = backend_query(b_idx, r_write);
			fail_unless(ret > 0,    "chain 'real_store_idx_str': write array failed");
		
		//printf("writing: ret: %x, ptr: %d, str: %s\n", ret, (unsigned int)data_ptrs[i], data_array[i]);
	}
	
	// check
	char data_read[1024], data_last[1024];
	
	memset(data_last, 0, 1024);
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		memset(data_read, 0, 1024);
		
		request_t r_read[] = {
			{ "action", DATA_INT32(ACTION_CRWD_READ)      },
			{ "key",    DATA_OFFT(i)                      },
			{ "buffer", DATA_PTR_STRING(&data_read, 1024) },
			hash_end
		};
		ret = backend_query(b_idx, r_read);
			fail_unless(ret > 0,                                "chain 'real_store_idx_str': read array failed");
			fail_unless(memcmp(data_read, data_last, 1024) > 0, "chain 'real_store_idx_str': sort array failed");
		
		//printf("reading: ret: %x, ptr: %d, str: %s\n", ret, i, data_read);
		
		memcpy(data_last, data_read, 1024);
	}
	
	backend_destroy(b_idx);
	backend_destroy(b_data);
}
END_TEST
REGISTER_TEST(core, test_real_store_idx_strings)
