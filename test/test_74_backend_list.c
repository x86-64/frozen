
START_TEST (test_backend_list){
	backend_t *backend;
	
	hash_t  settings[] = {
                { 0, DATA_HASHT(
                        { HK(class),        DATA_STRING("file")                     },
                        { HK(filename),     DATA_STRING("data_backend_list.dat")    },
                        hash_end
                )},
                { 0, DATA_HASHT(
                        { HK(class),        DATA_STRING("list")                     },
                        hash_end
                )},
                hash_end
	};
		
	backend = backend_new(settings);
		fail_unless(backend != NULL, "backend creation failed");
	
	ssize_t       ssize;
	
	off_t         key_off;
	char          key_data[]     = "0123456789";
	char          key_insert[]   = "a";
	char          key_inserted[] = "01a23456789";
	char          key_delete[]   = "01a3456789";
	char          temp[1024];
	
	// create new
	hash_t  hash_create[] = {
		{ HK(action),  DATA_UINT32T(ACTION_CRWD_CREATE) },
		{ HK(size),    DATA_SIZET(10)                 },
		{ HK(offset_out), DATA_PTR_OFFT(&key_off)     },
		hash_end
	};
	if( (ssize = backend_query(backend, hash_create)) < 0 )
		fail("backend in_list create failed");	
	
	// write keys
	hash_t  hash_write[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_WRITE)  },
		{ HK(offset), DATA_OFFT(key_off)             },
		{ HK(size),   DATA_SIZET(10)                 },
		{ HK(buffer), DATA_RAW(key_data, 10)         },
		hash_end
	};
	ssize = backend_query(backend, hash_write);
		fail_unless(ssize == 10, "backend in_list write 1 failed");
	
	// insert key
	hash_t  hash_insert[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_WRITE) },
		{ HK(offset), DATA_OFFT(key_off + 2)        },
		{ HK(insert), DATA_UINT32T(1)                 },
		{ HK(size),   DATA_SIZET(1)                 },
		{ HK(buffer), DATA_RAW(key_insert, 1)       },
		hash_end
	};
	ssize = backend_query(backend, hash_insert);
		fail_unless(ssize == 1,  "backend in_list write 2 failed");
	
	// check
	memset(temp, 0, 1024);
	hash_t  hash_read[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_READ)  },
		{ HK(offset), DATA_OFFT(key_off)            },
		{ HK(size),   DATA_SIZET(11)                },
		{ HK(buffer), DATA_RAW(temp, 1024)          },
		hash_end
	};
	ssize = backend_query(backend, hash_read);
		fail_unless(ssize == 11,                                "backend in_list read 1 failed");
		fail_unless(
			memcmp(temp, key_inserted, ssize) == 0,
			"backend in_list read 1 data failed"
		);
	
	// delete key
	hash_t  hash_delete[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_DELETE)   },
		{ HK(offset),    DATA_OFFT(key_off + 3)           },
		{ HK(size),   DATA_SIZET(1)                    },
		hash_end
	};
	ssize = backend_query(backend, hash_delete);
		fail_unless(ssize == 0,  "backend in_list delete failed");
	
	// check
	hash_t  hash_read2[] = {
		{ HK(action), DATA_UINT32T(ACTION_CRWD_READ)     },
		{ HK(offset), DATA_OFFT(key_off)               },
		{ HK(size),   DATA_SIZET(10)                   },
		{ HK(buffer), DATA_RAW(temp, 1024)             },
		hash_end
	};
	ssize = backend_query(backend, hash_read2);
		fail_unless(ssize == 10,                                "backend in_list read 1 failed");
		fail_unless(
			memcmp(temp, key_delete, 10) == 0,
			"backend in_list read 1 data failed"
		);
	
	backend_destroy(backend);
}
END_TEST
REGISTER_TEST(core, test_backend_list)

