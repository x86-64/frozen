
START_TEST (test_real_store_idx_strings){
	ssize_t  ret;
	
	hash_t    *c_idx = configs_file_parse("test_99_real_idx_str.conf");
		fail_unless(c_idx != NULL, "backend real_store_idx_str config parse failed");
	backend_t *b_idx = backend_new(c_idx);
		fail_unless(b_idx != NULL,      "backend real_store_idx_str backends create failed");
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
	
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		request_t r_write[] = {
			{ HK(action),     DATA_UINT32T(ACTION_CRWD_WRITE)                         },
			{ HK(offset_out), DATA_PTR_OFFT(&data_ptrs[i])                            },
			{ HK(buffer),     DATA_PTR_STRING(data_array[i], strlen(data_array[i])+1) },
			{ HK(size),       DATA_SIZET(strlen(data_array[i])+1)                     },
			{ HK(ret),        DATA_PTR_SIZET(&ret)                                    },
                        hash_end
		};
		backend_query(b_idx, r_write);
			fail_unless(ret >= 0,    "backend real_store_idx_str: write array failed");
		
		//printf("writing: ret: %x, ptr: %d, str: %s\n", ret, (unsigned int)data_ptrs[i], data_array[i]);
	}
	
	// check
	char data_read[1024], data_last[1024];
	
	memset(data_last, 0, 1024);
	for(i=0; i < sizeof(data_array) / sizeof(char *); i++){
		memset(data_read, 0, 1024);
		
		request_t r_read[] = {
			{ HK(action), DATA_UINT32T(ACTION_CRWD_READ)    },
			{ HK(offset), DATA_OFFT(i)                      },
			{ HK(buffer), DATA_PTR_STRING(&data_read, 1024) },
			{ HK(ret),    DATA_PTR_SIZET(&ret)              },
                        hash_end
		};
		backend_query(b_idx, r_read);
			fail_unless(ret > 0,                                "backend real_store_idx_str: read array failed");
			fail_unless(memcmp(data_read, data_last, 1024) > 0, "backend real_store_idx_str: sort array failed");
		
		//printf("reading: ret: %x, ptr: %d, str: %s\n", ret, i, data_read);
		
		memcpy(data_last, data_read, 1024);
	}
	
	backend_destroy(b_idx);
}
END_TEST
REGISTER_TEST(core, test_real_store_idx_strings)
