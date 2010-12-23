#include <test.h>

START_TEST (test_real_store_nums){
	hash_t config[] = {
		{ "chains", DATA_HASHT(
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
		{ "chains", DATA_HASHT(
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
	hash_t     *c_data = configs_string_parse(
		"name  =>'b_data',"
		"chains=>{ "
			"NULL => { name='file', filename='data_real_store_dat_string.dat' }"
		"}"
	);
	backend_t  *b_data = backend_new(c_data);
	hash_free(c_data);
	
	hash_t     *c_idx = configs_file_parse("config_simple_idx.conf");
		fail_unless(c_idx != NULL, "chain 'real_store_idx_str' config parse failed");
	backend_t  *b_idx = backend_new(c_idx);
		fail_unless(b_idx != NULL, "chain 'real_store_idx_str' backend create failed");
	hash_free(c_idx);
	
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
